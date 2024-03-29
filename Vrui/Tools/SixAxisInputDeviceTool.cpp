/***********************************************************************
SixAxisInputDeviceTool - Class for tools using six valuators for
translational and rotational axes to control virtual input devices.
Copyright (c) 2010 Oliver Kreylos

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

#include <Vrui/Tools/SixAxisInputDeviceTool.h>

#include <Misc/ThrowStdErr.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/CompoundValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Geometry/GeometryValueCoders.h>
#include <Vrui/Vrui.h>
#include <Vrui/ToolManager.h>

namespace Vrui {

/**********************************************
Methods of class SixAxisInputDeviceToolFactory:
**********************************************/

SixAxisInputDeviceToolFactory::SixAxisInputDeviceToolFactory(ToolManager& toolManager)
	:ToolFactory("SixAxisInputDeviceTool",toolManager),
	 selectButtonToggle(true)
	{
	/* Initialize tool layout: */
	layout.setNumButtons(1,true);
	layout.setNumValuators(6,true);
	
	/* Insert class into class hierarchy: */
	ToolFactory* inputDeviceToolFactory=toolManager.loadClass("InputDeviceTool");
	inputDeviceToolFactory->addChildClass(this);
	addParentClass(inputDeviceToolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	selectButtonToggle=cfs.retrieveValue<bool>("./selectButtonToggle",selectButtonToggle);
	
	typedef std::vector<Vector> VectorList;
	
	/* Initialize translation vectors: */
	Scalar translateFactor=cfs.retrieveValue<Scalar>("./translateFactor",getDisplaySize()/Scalar(3));
	VectorList translationVectors;
	for(int i=0;i<3;++i)
		{
		Vector t=Vector::zero;
		t[i]=Scalar(1);
		translationVectors.push_back(t);
		}
	translationVectors=cfs.retrieveValue<VectorList>("./translationVectors",translationVectors);
	if(translationVectors.size()!=3)
		Misc::throwStdErr("SixAxisInputDeviceToolFactory: wrong number of translation vectors; got %u, needed 3",(unsigned int)translationVectors.size());
	
	for(int i=0;i<3;++i)
		translations[i]=translationVectors[i]*translateFactor;
	
	/* Initialize rotation axes: */
	Scalar rotateFactor=Math::rad(cfs.retrieveValue<Scalar>("./rotateFactor",Scalar(180)));
	VectorList scaledRotationAxes;
	for(int i=0;i<3;++i)
		{
		Vector r=Vector::zero;
		r[i]=Scalar(1);
		scaledRotationAxes.push_back(r);
		}
	scaledRotationAxes=cfs.retrieveValue<VectorList>("./scaledRotationAxes",scaledRotationAxes);
	if(scaledRotationAxes.size()!=3)
		Misc::throwStdErr("SixAxisInputDeviceToolFactory: wrong number of rotation axes; got %u, needed 3",(unsigned int)scaledRotationAxes.size());
	
	for(int i=0;i<3;++i)
		rotations[i]=scaledRotationAxes[i]*rotateFactor;
	
	/* Set tool class' factory pointer: */
	SixAxisInputDeviceTool::factory=this;
	}

SixAxisInputDeviceToolFactory::~SixAxisInputDeviceToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	SixAxisInputDeviceTool::factory=0;
	}

const char* SixAxisInputDeviceToolFactory::getName(void) const
	{
	return "Six-Axis Driver";
	}

const char* SixAxisInputDeviceToolFactory::getButtonFunction(int buttonSlotIndex) const
	{
	switch(buttonSlotIndex)
		{
		case 0:
			return "Select Device";
		
		default:
			return "Forwarded Button";
		}
	}

const char* SixAxisInputDeviceToolFactory::getValuatorFunction(int valuatorSlotIndex) const
	{
	switch(valuatorSlotIndex)
		{
		case 0:
			return "Translate Vector 0";
		
		case 1:
			return "Translate Vector 1";
		
		case 2:
			return "Translate Vector 2";
		
		case 3:
			return "Rotate Axis 0";
		
		case 4:
			return "Rotate Axis 1";
		
		case 5:
			return "Rotate Axis 2";
		
		default:
			return "Forwarded Valuator";
		}
	}

Tool* SixAxisInputDeviceToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new SixAxisInputDeviceTool(this,inputAssignment);
	}

void SixAxisInputDeviceToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveSixAxisInputDeviceToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("InputDeviceTool");
	}

extern "C" ToolFactory* createSixAxisInputDeviceToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	SixAxisInputDeviceToolFactory* sixAxisInputDeviceToolFactory=new SixAxisInputDeviceToolFactory(*toolManager);
	
	/* Return factory object: */
	return sixAxisInputDeviceToolFactory;
	}

extern "C" void destroySixAxisInputDeviceToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/***********************************************
Static elements of class SixAxisInputDeviceTool:
***********************************************/

SixAxisInputDeviceToolFactory* SixAxisInputDeviceTool::factory=0;

/***************************************
Methods of class SixAxisInputDeviceTool:
***************************************/

SixAxisInputDeviceTool::SixAxisInputDeviceTool(const ToolFactory* sFactory,const ToolInputAssignment& inputAssignment)
	:InputDeviceTool(sFactory,inputAssignment)
	{
	/* Set the interaction device: */
	interactionDevice=getButtonDevice(0);
	}

const ToolFactory* SixAxisInputDeviceTool::getFactory(void) const
	{
	return factory;
	}

void SixAxisInputDeviceTool::buttonCallback(int buttonSlotIndex,InputDevice::ButtonCallbackData* cbData)
	{
	if(buttonSlotIndex==0)
		{
		if(cbData->newButtonState) // Button has just been pressed
			{
			/* Deactivate the tool: */
			deactivate();
			
			/* Calculate an interaction ray: */
			Ray interactionRay=calcInteractionRay();
			
			/* Try activating the tool: */
			activate(interactionRay);
			}
		}
	else
		{
		/* Let input device tool handle it: */
		InputDeviceTool::buttonCallback(buttonSlotIndex,cbData);
		}
	}

void SixAxisInputDeviceTool::frame(void)
	{
	if(isActive())
		{
		/* Assemble translation from translation vectors and current valuator values: */
		Vector translation=Vector::zero;
		for(int i=0;i<3;++i)
			translation+=factory->translations[i]*Scalar(getValuatorState(i));
		translation*=getCurrentFrameTime();
		
		/* Assemble rotation from scaled rotation axes and current valuator values: */
		Vector rotation=Vector::zero;
		for(int i=0;i<3;++i)
			rotation+=factory->rotations[i]*Scalar(getValuatorState(3+i));
		rotation*=getCurrentFrameTime();
		
		/* Calculate an incremental transformation for the virtual input device: */
		ONTransform deltaT=ONTransform::translate(translation);
		Point pos=getGrabbedDevice()->getPosition();
		deltaT*=ONTransform::translateFromOriginTo(pos);
		deltaT*=ONTransform::rotate(ONTransform::Rotation::rotateScaledAxis(rotation));
		deltaT*=ONTransform::translateToOriginFrom(pos);
	
		/* Update the virtual input device's transformation: */
		deltaT*=getGrabbedDevice()->getTransformation();
		deltaT.renormalize();
		getGrabbedDevice()->setTransformation(deltaT);
		
		/* Request another frame if the input device has moved: */
		if(translation!=Vector::zero||rotation!=Vector::zero)
			{
			/* Request another frame: */
			scheduleUpdate(getApplicationTime()+1.0/125.0);
			}
		}
	}

}
