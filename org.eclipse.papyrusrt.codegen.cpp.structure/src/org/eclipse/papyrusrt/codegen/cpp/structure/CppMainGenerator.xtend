/*******************************************************************************
 * Copyright (c) 2014-2015 Zeligsoft (2009) Limited and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *******************************************************************************/
package org.eclipse.papyrusrt.codegen.cpp.structure

import java.io.BufferedWriter
import java.io.File
import java.io.FileWriter
import org.eclipse.papyrusrt.codegen.cpp.structure.model.Deployment
import org.eclipse.papyrusrt.codegen.lang.cpp.element.Variable

class CppMainGenerator {

	def generate(String filePath, String topName, Deployment deployment, Variable slotsVariable) {
		
		val file = new File(filePath);

		val writer = new BufferedWriter(new FileWriter(file));
		writer.write(doGenerate(topName, deployment, slotsVariable).toString)
		writer.close
	}

	def private doGenerate(String topName, Deployment deployment, Variable slotsVariable) {
		'''
#include "umlrtmain.hh"

#include "umlrtcontroller.hh"
#include "«topName»Slots.hh"
#include "umlrtdeploymentmap.hh"
#include "umlrtmessagepool.hh"
#include "umlrtsignalelementpool.hh"
#include "umlrttimerpool.hh"
#include "umlrtuserconfig.hh"
#include "umlrtexecutiondirector.hh"
#include <stdio.h>

static UMLRTSignalElement signalElementBuffer[USER_CONFIG_SIGNAL_ELEMENT_POOL_SIZE];
static UMLRTSignalElementPool signalElementPool( signalElementBuffer, USER_CONFIG_SIGNAL_ELEMENT_POOL_SIZE );

static UMLRTMessage messageBuffer[USER_CONFIG_MESSAGE_POOL_SIZE];
static UMLRTMessagePool messagePool( messageBuffer, USER_CONFIG_MESSAGE_POOL_SIZE );

static UMLRTTimer timers[USER_CONFIG_TIMER_POOL_SIZE];
static UMLRTTimerPool timerPool( timers, USER_CONFIG_TIMER_POOL_SIZE );

int main( int argc, char * argv[] )
{
    UMLRTController::initializePools( &signalElementPool, &messagePool, &timerPool );
    UMLRTMain::setArgs( argc, argv );
    UMLRTDeploymentMap::setDefaultSlotList( «slotsVariable.name.identifier», «slotsVariable.numInitializedInstances» );
	UMLRTExecutionDirector::init( UMLRTMain::address);
	
    if( ! UMLRTMain::targetStartup() )
        return EXIT_FAILURE;

	UMLRTExecutionDirector::spawn();
	
    if( ! UMLRTMain::mainLoop() )
        return UMLRTMain::targetShutdown( false );

	UMLRTExecutionDirector::join();

    return UMLRTMain::targetShutdown( true );
}
'''
	}
}
