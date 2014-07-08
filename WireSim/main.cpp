/***
 
 WireSim: Esoteric Computer built out of discrete gates, wholly simulated with hand-made images, execute through graphics cards!
 
 Gates: (4 colors) NOT, AND, OR, XOR; takes 2 from any corner, sends signal to directly adjacent
 
 Writes: (2 colors) Green, orange; spreads power directly adjacent on same-color wires, has
 four states of power, which spreads with full power to state 0, always decrementing
 
 Junctions: (2 colors): Yellow, Brown: (y)connects all directly adjacent wires, (b)top-to-bottom, left-to-right
 
 Could take language like VHDL and convert it to graphics image...
 
 Implement as GLSL; so either we do 4 taps, 4 writes
 
 1. Read center tile
 
 2. If gate:
 If no power
 Read all corners
 if logic_func( all valid corners ) is true, then raise power high,
 Dissipate to directly adjacent no-power tiles
 Power--
 
 3. If wire:
 Dissipate to directly adjacent same-color wire
 Power--
 
 4. If junction:
 Move power state from above to below, left-to-right, top-to-bottom
 Or spread all power
 
 This is to be implemented:
    Single-Core CPU
    Multi-Core CPU
    GPU
    OpenCL
    FPGA
 
***/

#include <stdio.h>

#include "WireSim.h"

// Main application entry point
int main()
{
    // Simulation cycle count
    const int cMaxSimulationCount = 1000;
    const int cPixelSize = 8;
    
    // All circuits to simulate
    const int cPngFileNameCount = 1;//14;
    const char* cPngFileNames[ cPngFileNameCount ] =
    {
        //"StraightWireGreen.png",
        //"StraightWireOrange.png",
        //"StraightWires.png",
        //"WirePair_16Full.png",
        //"WirePair_128Full.png",
        //"JumpJointTests.png",
        //"MergeJointTests.png",
        //"NotGateTests.png",
        //"AndGateTests.png",
        //"OrGateTests.png",
        "XorGateTests.png",
        //"WireOverlap.png",
        //"Circuit_2To4Decoder_v2.png",
        //"Circuit_2To4Decoder_v3.png",
    };
    
    for( int i = 0; i < cPngFileNameCount; i++ )
    {
        printf( "Starting simulations for \"%s\":\n", cPngFileNames[ i ] );
        
        WireSim wireSim( cPngFileNames[ i ] );
        
        // File name buffer
        char fileName[ 512 ];
        
        // Initial state
        sprintf( fileName, "%s_000.output.png", cPngFileNames[ i ] );
        wireSim.SaveState( fileName, cPixelSize );
        
        for( int j = 1; j < cMaxSimulationCount; j++ )
        {
            // Testing states
            for( int k = 0; k < wireSim.GetInputCount(); k++ )
            {
                wireSim.SetInput( k, true );
            }
            
            bool hasChanged = wireSim.Update();
            
            sprintf( fileName, "%s_%03d.output.png", cPngFileNames[ i ], j );
            fflush( stdout );
            wireSim.SaveState( fileName, cPixelSize, true );
            
            printf( " %0.1f%%", 100.0f * ( float(j + 1) / float( cMaxSimulationCount ) ) );
            
            if( hasChanged == false )
            {
                break;
            }
        }
        
        printf( " Done!\n" );
    }
        
    return 0;
}
