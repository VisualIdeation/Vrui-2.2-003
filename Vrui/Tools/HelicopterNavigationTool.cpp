/***********************************************************************
HelicopterNavigationTool - Class for navigation tools using a simplified
helicopter flight model, a la Enemy Territory: Quake Wars' Anansi. Yeah,
I like that -- wanna fight about it?
Copyright (c) 2007-2011 Oliver Kreylos

This file is part of the Virtual Reality User Interface Library (Vrui).

The Virtual Reality User Interface Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Virtual Reality User Interface Library is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Virtual Reality User Interface Library; if not, write to the
Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#include <Vrui/Tools/HelicopterNavigationTool.h>

#include <Misc/Utility.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Geometry/GeometryValueCoders.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLValueCoders.h>
#include <GL/GLContextData.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/Viewer.h>
#include <Vrui/ToolManager.h>

namespace Vrui {

/************************************************
Methods of class HelicopterNavigationToolFactory:
************************************************/

HelicopterNavigationToolFactory::HelicopterNavigationToolFactory(ToolManager& toolManager)
	:ToolFactory("HelicopterNavigationTool",toolManager),
	 g(getMeterFactor()*Scalar(9.81)),
	 collectiveMin(Scalar(0)),collectiveMax(g*Scalar(1.5)),
	 thrust(g*Scalar(1)),
	 brake(g*Scalar(0.5)),
	 probeSize(getMeterFactor()*Scalar(1.5)),
	 maxClimb(getMeterFactor()*Scalar(1.5)),
	 drawHud(true),hudColor(0.0f,1.0f,0.0f),hudRadius(float(getFrontplaneDist()*1.25f)),hudFontSize(float(getUiSize())*1.5f)
	{
	/* Initialize tool layout: */
	layout.setNumButtons(3);
	layout.setNumValuators(6);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	Vector rot=cfs.retrieveValue<Vector>("./rotateFactors",Vector(-60,-60,45));
	for(int i=0;i<3;++i)
		rotateFactors[i]=Math::rad(rot[i]);
	g=cfs.retrieveValue<Scalar>("./g",g);
	collectiveMin=cfs.retrieveValue<Scalar>("./collectiveMin",collectiveMin);
	collectiveMax=cfs.retrieveValue<Scalar>("./collectiveMax",collectiveMax);
	thrust=cfs.retrieveValue<Scalar>("./thrust",thrust);
	brake=cfs.retrieveValue<Scalar>("./brake",brake);
	Vector drag=cfs.retrieveValue<Vector>("./dragCoefficients",Vector(0.3,0.1,0.3));
	for(int i=0;i<3;++i)
		dragCoefficients[i]=-Math::abs(drag[i]);
	Geometry::Vector<Scalar,2> view=cfs.retrieveValue<Geometry::Vector<Scalar,2> >("./viewAngleFactors",Geometry::Vector<Scalar,2>(35,-25));
	for(int i=0;i<2;++i)
		viewAngleFactors[i]=Math::rad(view[i]);
	probeSize=cfs.retrieveValue<Scalar>("./probeSize",probeSize);
	maxClimb=cfs.retrieveValue<Scalar>("./maxClimb",maxClimb);
	drawHud=cfs.retrieveValue<bool>("./drawHud",drawHud);
	hudColor=cfs.retrieveValue<Color>("./hudColor",hudColor);
	hudRadius=cfs.retrieveValue<float>("./hudRadius",hudRadius);
	hudFontSize=cfs.retrieveValue<float>("./hudFontSize",hudFontSize);
	
	/* Insert class into class hierarchy: */
	ToolFactory* navigationToolFactory=toolManager.loadClass("SurfaceNavigationTool");
	navigationToolFactory->addChildClass(this);
	addParentClass(navigationToolFactory);
	
	/* Set tool class' factory pointer: */
	HelicopterNavigationTool::factory=this;
	}

HelicopterNavigationToolFactory::~HelicopterNavigationToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	HelicopterNavigationTool::factory=0;
	}

const char* HelicopterNavigationToolFactory::getName(void) const
	{
	return "Helicopter Flight";
	}

const char* HelicopterNavigationToolFactory::getButtonFunction(int buttonSlotIndex) const
	{
	switch(buttonSlotIndex)
		{
		case 0:
			return "Start / Stop";
		
		case 1:
			return "Thrusters";
		
		case 2:
			return "Brake";
		}
	
	/* Never reached; just to make compiler happy: */
	return 0;
	}

const char* HelicopterNavigationToolFactory::getValuatorFunction(int valuatorSlotIndex) const
	{
	switch(valuatorSlotIndex)
		{
		case 0:
			return "Cyclic Pitch";
		
		case 1:
			return "Cyclic Roll";
		
		case 2:
			return "Rudder Yaw";
		
		case 3:
			return "Collective";
		
		case 4:
			return "Look Left/Right";
		
		case 5:
			return "Look Up/Down";
		}
	
	/* Never reached; just to make compiler happy: */
	return 0;
	}

Tool* HelicopterNavigationToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new HelicopterNavigationTool(this,inputAssignment);
	}

void HelicopterNavigationToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveHelicopterNavigationToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("SurfaceNavigationTool");
	}

extern "C" ToolFactory* createHelicopterNavigationToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	HelicopterNavigationToolFactory* helicopterNavigationToolFactory=new HelicopterNavigationToolFactory(*toolManager);
	
	/* Return factory object: */
	return helicopterNavigationToolFactory;
	}

extern "C" void destroyHelicopterNavigationToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/*************************************************
Static elements of class HelicopterNavigationTool:
*************************************************/

HelicopterNavigationToolFactory* HelicopterNavigationTool::factory=0;

/*****************************************
Methods of class HelicopterNavigationTool:
*****************************************/

void HelicopterNavigationTool::applyNavState(void)
	{
	/* Compose and apply the navigation transformation: */
	NavTransform nav=physicalFrame;
	nav*=NavTransform::rotate(Rotation::rotateZ(getValuatorState(4)*factory->viewAngleFactors[0]));
	nav*=NavTransform::rotate(Rotation::rotateX(getValuatorState(5)*factory->viewAngleFactors[1]));
	nav*=NavTransform::rotate(orientation);
	nav*=Geometry::invert(surfaceFrame);
	setNavigationTransformation(nav);
	}

void HelicopterNavigationTool::initNavState(void)
	{
	/* Set up a physical navigation frame around the main viewer's current head position: */
	calcPhysicalFrame(getMainViewer()->getHeadPosition());
	
	/* Calculate the initial environment-aligned surface frame in navigation coordinates: */
	surfaceFrame=getInverseNavigationTransformation()*physicalFrame;
	NavTransform newSurfaceFrame=surfaceFrame;
	
	/* Align the initial frame with the application's surface: */
	AlignmentData ad(surfaceFrame,newSurfaceFrame,factory->probeSize,factory->maxClimb);
	align(ad);
	
	/* Calculate the orientation of the current navigation transformation in the aligned surface frame: */
	orientation=Geometry::invert(surfaceFrame.getRotation())*newSurfaceFrame.getRotation();
	
	/* Reset the movement velocity: */
	velocity=Vector::zero;
	
	/* If the initial surface frame was above the surface, lift it back up: */
	elevation=newSurfaceFrame.inverseTransform(surfaceFrame.getOrigin())[2];
	if(elevation<factory->probeSize)
		{  
		/* Collide with the ground: */
		elevation=factory->probeSize;
		Vector y=orientation.getDirection(1);
		Scalar azimuth=Math::atan2(y[0],y[1]);
		orientation=Rotation::rotateZ(-azimuth);
		}
	newSurfaceFrame*=NavTransform::translate(Vector(Scalar(0),Scalar(0),elevation));
	
	/* Apply the initial navigation state: */
	surfaceFrame=newSurfaceFrame;
	applyNavState();
	}

HelicopterNavigationTool::HelicopterNavigationTool(const ToolFactory* sFactory,const ToolInputAssignment& inputAssignment)
	:SurfaceNavigationTool(sFactory,inputAssignment),
	 numberRenderer(factory->hudFontSize,true)
	{
	}

const ToolFactory* HelicopterNavigationTool::getFactory(void) const
	{
	return factory;
	}

void HelicopterNavigationTool::buttonCallback(int buttonSlotIndex,InputDevice::ButtonCallbackData* cbData)
	{
	/* Process based on which button was pressed: */
	if(buttonSlotIndex==0)
		{
		if(cbData->newButtonState)
			{
			/* Act depending on this tool's current state: */
			if(isActive())
				{
				/* Deactivate this tool: */
				deactivate();
				}
			else
				{
				/* Try activating this tool: */
				if(activate())
					{
					/* Initialize the navigation: */
					initNavState();
					}
				}
			}
		}
	}

void HelicopterNavigationTool::frame(void)
	{
	/* Act depending on this tool's current state: */
	if(isActive())
		{
		/* Use the average frame time as simulation time: */
		Scalar dt=getCurrentFrameTime();
		
		/* Update the current position based on the current velocity: */
		NavTransform newSurfaceFrame=surfaceFrame;
		newSurfaceFrame*=NavTransform::translate(velocity*dt);
		
		/* Re-align the surface frame with the surface: */
		Point initialOrigin=newSurfaceFrame.getOrigin();
		AlignmentData ad(surfaceFrame,newSurfaceFrame,factory->probeSize,factory->maxClimb);
		align(ad);
		
		/* Update the orientation to reflect rotations in the surface frame: */
		orientation*=Geometry::invert(surfaceFrame.getRotation())*newSurfaceFrame.getRotation();
		
		/* Check if the initial surface frame was above the surface: */
		elevation=newSurfaceFrame.inverseTransform(initialOrigin)[2];
		if(elevation<factory->probeSize)
			{
			/* Collide with the ground: */
			elevation=factory->probeSize;
			Vector y=orientation.getDirection(1);
			Scalar azimuth=Math::atan2(y[0],y[1]);
			orientation=Rotation::rotateZ(-azimuth);
			velocity=Vector::zero;
			}
		
		/* Lift the aligned frame back up to the original altitude: */
		newSurfaceFrame*=NavTransform::translate(Vector(Scalar(0),Scalar(0),elevation));
		
		/* Update the current orientation based on the pitch, roll, and yaw controls: */
		Vector rot;
		for(int i=0;i<3;++i)
			rot[i]=getValuatorState(i)*factory->rotateFactors[i];
		orientation.leftMultiply(Rotation::rotateScaledAxis(rot*dt));
		orientation.renormalize();
		
		/* Calculate the current acceleration based on gravity, collective, thrust, and brake: */
		Vector accel=Vector(0,0,-factory->g);
		Scalar collective=Scalar(0.5)*(Scalar(1)-getValuatorState(3))*(factory->collectiveMax-factory->collectiveMin)+factory->collectiveMin;
		accel+=orientation.inverseTransform(Vector(0,0,collective));
		if(getButtonState(1))
			accel+=orientation.inverseTransform(Vector(0,factory->thrust,0));
		if(getButtonState(2))
			accel+=orientation.inverseTransform(Vector(0,-factory->brake,0));
		
		/* Calculate drag: */
		Vector localVelocity=orientation.transform(velocity);
		Vector drag;
		for(int i=0;i<3;++i)
			drag[i]=localVelocity[i]*factory->dragCoefficients[i];
		accel+=orientation.inverseTransform(drag);
		
		/* Update the current velocity: */
		velocity+=accel*dt;
		
		/* Apply the newly aligned surface frame: */
		surfaceFrame=newSurfaceFrame;
		applyNavState();
		
		/* Request another frame: */
		scheduleUpdate(getApplicationTime()+1.0/125.0);
		}
	}

void HelicopterNavigationTool::display(GLContextData& contextData) const
	{
	if(isActive()&&factory->drawHud)
		{
		glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
		glDisable(GL_LIGHTING);
		glLineWidth(1.0f);
		glColor(factory->hudColor);
		
		/* Get the HUD layout parameters: */
		float y=factory->hudRadius;
		float s=factory->hudFontSize;
		
		/* Go to the view-shifted physical frame: */
		glPushMatrix();
		glMultMatrix(physicalFrame);
		glRotate(getValuatorState(4)*Math::deg(factory->viewAngleFactors[0]),Vector(0,0,1));
		glRotate(getValuatorState(5)*Math::deg(factory->viewAngleFactors[1]),Vector(1,0,0));
		
		/* Go to the HUD frame: */
		glTranslatef(0.0f,y,0.0f);
		glRotatef(90.0f,1.0f,0.0f,0.0f);
		
		/* Draw the boresight crosshairs: */
		glBegin(GL_LINES);
		glVertex2f(-y*0.02f,   0.00f);
		glVertex2f(-y*0.01f,   0.00f);
		glVertex2f( y*0.01f,   0.00f);
		glVertex2f( y*0.02f,   0.00f);
		glVertex2f(   0.00f,-y*0.02f);
		glVertex2f(   0.00f,-y*0.01f);
		glVertex2f(   0.00f, y*0.01f);
		glVertex2f(   0.00f, y*0.02f);
		glEnd();
		
		/* Get the helicopter's orientation Euler angles: */
		Scalar angles[3];
		calcEulerAngles(orientation,angles);
		float azimuth=Math::deg(angles[0]);
		float elevation=Math::deg(angles[1]);
		float roll=Math::deg(angles[2]);
		
		/* Draw the compass ribbon: */
		glBegin(GL_LINES);
		glVertex2f(-y*0.5f,y*0.5f);
		glVertex2f(y*0.5f,y*0.5f);
		glEnd();
		glBegin(GL_LINE_STRIP);
		glVertex2f(-s,y*0.5f+s*2.0f);
		glVertex2f(0.0f,y*0.5f);
		glVertex2f(s,y*0.5f+s*2.0f);
		glEnd();
		
		/* Draw the azimuth tick marks: */
		glBegin(GL_LINES);
		for(int az=0;az<360;az+=10)
			{
			float dist=float(az)-azimuth;
			if(dist<-180.0f)
				dist+=360.0f;
			if(dist>180.0f)
				dist-=360.0f;
			if(Math::abs(dist)<=60.0f)
				{
				float x=dist*y*0.5f/60.0f;
				glVertex2f(x,y*0.5f);
				glVertex2f(x,y*0.5f-(az%30==0?s*1.5f:s));
				}
			}
		glEnd();
		
		/* Draw the azimuth labels: */
		GLNumberRenderer::Vector pos;
		pos[1]=y*0.5f-s*2.0f;
		pos[2]=0.0f;
		for(int az=0;az<360;az+=30)
			{
			float dist=float(az)-azimuth;
			if(dist<-180.0f)
				dist+=360.0f;
			if(dist>180.0f)
				dist-=360.0f;
			if(Math::abs(dist)<=60.0f)
				{
				pos[0]=dist*y*0.5f/60.0f;
				numberRenderer.drawNumber(pos,az,contextData,0,1);
				}
			}
		
		/* Draw the flight path marker: */
		Vector vel=orientation.transform(velocity);
		if(vel[1]>Scalar(0))
			{
			vel*=y/vel[1];
			Scalar maxVel=Misc::max(Math::abs(vel[0]),Math::abs(vel[2]));
			if(maxVel>=Scalar(y*0.5f))
				{
				vel[0]*=Scalar(y*0.5f)/maxVel;
				vel[2]*=Scalar(y*0.5f)/maxVel;
				glColor3f(1.0f,0.0f,0.0f);
				}
			
			glBegin(GL_LINE_LOOP);
			glVertex2f(vel[0]-y*0.005f,vel[2]+  0.000f);
			glVertex2f(vel[0]+  0.000f,vel[2]-y*0.005f);
			glVertex2f(vel[0]+y*0.005f,vel[2]+  0.000f);
			glVertex2f(vel[0]+  0.000f,vel[2]+y*0.005f);
			glEnd();
			}
		
		glColor(factory->hudColor);
		
		glRotatef(-roll,0.0f,0.0f,1.0f);
		
		/* Draw the artificial horizon ladder: */
		glEnable(GL_LINE_STIPPLE);
		glLineStipple(10,0xaaaaU);
		glBegin(GL_LINES);
		for(int el=-175;el<0;el+=5)
			{
			float dist=elevation+float(el);
			if(dist<-180.0f)
				dist+=360.0f;
			if(dist>180.0f)
				dist-=360.0f;
			if(Math::abs(dist)<90.0f)
				{
				float z=Math::tan(Math::rad(dist))*y;
				if(Math::abs(z)<=y*0.5f)
					{
					float x=el%10==0?y*0.1f:y*0.05f;
					glVertex2f(-x,z);
					glVertex2f(x,z);
					}
				}
			}
		glEnd();
		glDisable(GL_LINE_STIPPLE);
		
		glBegin(GL_LINES);
		for(int el=0;el<=180;el+=5)
			{
			float dist=elevation+float(el);
			if(dist<-180.0f)
				dist+=360.0f;
			if(dist>180.0f)
				dist-=360.0f;
			if(Math::abs(dist)<90.0f)
				{
				float z=Math::tan(Math::rad(dist))*y;
				if(Math::abs(z)<=y*0.5f)
					{
					float x=el%10==0?y*0.1f:y*0.05f;
					glVertex2f(-x,z);
					glVertex2f(x,z);
					}
				}
			}
		glEnd();
		
		/* Draw the artificial horizon labels: */
		pos[0]=y*0.1f+s;
		pos[2]=0.0f;
		for(int el=-170;el<=180;el+=10)
			{
			float dist=elevation+float(el);
			if(dist<-180.0f)
				dist+=360.0f;
			if(dist>180.0f)
				dist-=360.0f;
			if(Math::abs(dist)<90.0f)
				{
				float z=Math::tan(Math::rad(dist))*y;
				if(Math::abs(z)<=y*0.5f)
					{
					pos[1]=z;
					int drawEl=el;
					if(drawEl>90)
						drawEl=180-el;
					else if(drawEl<-90)
						drawEl=-180-el;
					numberRenderer.drawNumber(pos,drawEl,contextData,-1,0);
					}
				}
			}
		
		glPopMatrix();
		glPopAttrib();
		}
	}

}
