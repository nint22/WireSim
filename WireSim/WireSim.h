/***
 
 WireSim - Discrete Circuit Simulated PixelArt
 Copyright (c) 2014 Jeremy Bridon

 Implements a WireSim simulation on the CPU. WireSim is a finite-state
 automaton that can be draw out in an image, using the Tango Icon Theme
 color template (see below for node definitions). These nodes are placed
 into a simple :
 
 Every element has an input and output pattern; wire-types recieve on
 directly adjacent-tiles and transmit back to directly-adjacent tiles.
 The Jumper moves from one side to the direct other, while gates operate
 on a Y-like input. Note that all entities only read / write from wires,
 so you can't chain any gate into a line.
 
 Orange: Wire 0
    Shares incoming signal changes
 Green: Wire 1
    Same as Wire 0, but does not share a signal with it
 Brown: Jumper
    Moves signals top-down, bottom-up, left-right, and
    right-left; can transmit across wire types.
 Red: And-Gate
    Accepts from any or all corners, transmitting to all
    directly-adjacent tiles.
 Blue: Or-Gate
 Yellow: Xor-Gate
 Purple: Not-Gate
 
 ***/

#ifndef __WIRESIM_H__
#define __WIRESIM_H__

#include <vector>
#include <stdint.h>

class WireSim
{
    
public:
    
    WireSim( const char* pngFileName );
    ~WireSim();
    
    // All types
    enum SimType
    {
        cSimType_None, // Any other color
        
        cSimType_WireType0,
        cSimType_WireType1,
        cSimType_JumpJoint,
        cSimType_AndGate,
        cSimType_OrGate,
        cSimType_XorGate,
        cSimType_NotGate,
        
        // Must always be last!
        cSimTypeCount
    };
    
    // Power types
    enum SimPower
    {
        cSimPower_LowEdge = 0,
        cSimPower_FallingEdge,
        cSimPower_HighEdge,
        cSimPower_RisingEdge,
        
        // Must always be last!
        cSimPowerCount
    };
    
    // Get size of the image
    void GetSize( int& widthOut, int& heightOut ) const;
    
    // Write to the inputs (left-column even-row pixels); note that you can only write 4-levels of power, [0,3]
    int GetInputCount() const;
    void SetInput( int inputIndex, bool turnOn );
    
    // Access to output pins (right-column even-row pixels
    int GetOutputCount() const;
    void GetOutput( int outputIndex, SimPower& powerLevelOut ) const;
    
    // Full simulation step; returns true if any pixel has changed state
    bool Update();
    
    // Save the current state of the PNG to the given filename; returns true on success, false on failure
    // If highlightEdgeChanges is set to true, then we draw a box outline on any edge-rise or edge-fall tiles
    bool SaveState( const char* pngOutFileName, int pixelSize = 1, bool highlightEdgeChanges = false );
    
    // Color type; ARGB format
    typedef uint32_t SimColor;
    
protected:
    
    // Bounds check
    inline bool IsBounded( int x, int y ) const;
    
    // Convert 2D position to linear index; top-left is origin (0,0), grows X+ to the right, Y+ down
    inline int GetLinearPosition( int x, int y ) const;
    
    // Simulate a single pixel
    void Update( int x, int y, const std::vector< SimColor >& source, std::vector< SimColor >& dest );
    
    // Get type and power value of the given color; power is added to each color component
    // Returns true if found, else returns false
    bool GetSimType( const SimColor& givenColor, SimType& simTypeOut, SimPower& powerOut ) const;
    bool GetSimType( int x, int y, SimType& simTypeOut, SimPower& powerOut ) const;
    
    // Set the simtype or power into the given buffer using linear indexing
    void SetSimType( std::vector< SimColor >& dstImage, int x, int y, const SimType& simType, SimPower powerLevel );
    
    // Create color with the appropriate tint (based on power level)
    SimColor MakeSimColor( const SimType& simType, SimPower powerLevel );
    
    // Fast inline filters
    inline bool IsEdge( const SimPower& simPower ) const;
    inline bool IsSettled( const SimPower& simPower ) const;
    inline bool IsWire( const SimType& simType ) const;
    inline bool IsGate( const SimType& simType ) const;
    
private:
    
    // Size of image
    int m_width, m_height;
    
    // List of input / outpout indices (wire pixels in the left and right column even-rows)
    std::vector< int > m_inputIndices;
    std::vector< int> m_outputIndices;
    
    // Current colors / states
    std::vector< SimColor > m_image;
    
};

#endif // __WIRESIM_H__
