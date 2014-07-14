/***
 
 WireSim - Discrete Circuit Simulated PixelArt
 Copyright (c) 2014 Jeremy Bridon
 
 Read in a scripting language that executes automated
 tests / simulation of a WireSim circuit. Syntax:
 
 The file must be an ASCII *.txt file, one command
 per line. The contents are not case-sensitive. Comments
 start for any text-line that beings with a hash-
 character '#'. A command can either be "set", "eval",
 and "test". Some commands take space-delimited arguments.
 
 set <input pin index> <0|1>: Sets the given input pint
 index to the 0 (low) or 1 (high) state.
 
 eval: Continues simulation until the system is fully
 simulated and can produce a valid output.
 
 test <input pin index> <0|1>: Tests if the given output
 pin index has the appropriate value of either 0 (low)
 or 1 (high).
 
***/
/*
#ifndef __TESTMANAGER_H__
#define __TESTMANAGER_H__

#include "Vec2.h"

class TestManager
{
    
public:
    
    TestManager( const char* fileName );
    ~TestManager();
    
    // Execute the test; returns zero on success, else returns the error count
    int Test();
    
protected:
    
    enum InstructionType
    {
        cInstructionType_Set,
        cInstructionType_Eval,
        cInstructionType_Test,
        
        // Must be last
        cInstructionTypeCount
    };
    
    struct TestInstruction
    {
        InstructionType m_instructionType;
        Vec2 m_instructionArgs;
    };
    
};

#endif
*/