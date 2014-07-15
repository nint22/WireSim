/***
 
 WireSim - Discrete Circuit Simulated PixelArt
 Copyright (c) 2014 Jeremy Bridon

 ***/

#include <vector>

#include "../lodepng.h"
#include "Vec2.h"
#include "WireSim.h"

namespace
{
    // Color constants; ARGB format
    // Based on Tango Icon Theme; two stable-states, two transition states, oredered:
    // Low edge, falling edge, high edge, rising edge
    const WireSim::SimColor cSimColors[ WireSim::cSimTypeCount ][ WireSim::cSimPowerCount ] =
    {
        { 0x00ffffff, 0x00ffffff, 0x00ffffff, 0x00ffffff }, // None-type
        { 0x00fcaf3e, 0x00fcaf4f, 0x00f57900, 0x00f57911 }, // Wire 0 (Orange)
        { 0x008ae234, 0x008ae245, 0x004e9a06, 0x004e9a17 }, // Wire 1 (Green)
        { 0x00e9b96e, 0x00e9b96f, 0x008f5902, 0x008f5903 }, // Jump Joint (Brown)
        { 0x00ef2929, 0x00ef292a, 0x00a40000, 0x00a40001 }, // And (Red)
        { 0x00729fcf, 0x00729fd0, 0x00204a87, 0x00204a88 }, // Or (Blue)
        { 0x00fce94f, 0x00fce950, 0x00c4a000, 0x00c4a001 }, // Xor (Yellow)
        { 0x00ad7fa8, 0x00ad7fa9, 0x005c3566, 0x005c3567 }, // Not (Purple)
    };
    
    // Constants used for iterating over adjacent tiles, etc..
    
    // Directly adjacent offsets
    const int cDirectlyAdjacentOffsetCount = 4;
    const Vec2 cDirectlyAdjacentOffsets[ cDirectlyAdjacentOffsetCount ] =
    {
        Vec2( 2, 1 ), // Right
        Vec2( 1, 2 ), // Down
        Vec2( 0, 1 ), // Left
        Vec2( 1, 0 ), // Top
    };
    
    // Corner offsets
    const int cCornerOffsetCount = 4;
    const Vec2 cCornerOffsets[ cCornerOffsetCount ] =
    {
        Vec2( 0, 0 ), // Top-left
        Vec2( 2, 0 ), // Top-right
        Vec2( 2, 2 ), // Bottom-right
        Vec2( 0, 2 ), // Bottom-left
    };
    
}

WireSim::WireSim( const char* pngFileName )
    : m_width( 0 )
    , m_height( 0 )
{
    std::vector< unsigned char > srcImage;
    unsigned int width;
    unsigned int height;
    
    // Given as RGBA format, convert to ARGB
    unsigned int error = lodepng::decode( srcImage, width, height, pngFileName );
    
    if( error != 0 )
    {
        printf( "Failed to load\n" );
        return;
    }
    
    m_width = (int)width;
    m_height = (int)height;
    
    for( int i = 0; i < (int)srcImage.size(); i += 4 )
    {
        SimColor simColor = 0;
        simColor |= (SimColor)( srcImage.at( i + 0 ) & 0xff ) << 16;
        simColor |= (SimColor)( srcImage.at( i + 1 ) & 0xff ) << 8;
        simColor |= (SimColor)( srcImage.at( i + 2 ) & 0xff );
        m_image.push_back( simColor );
    }
    
    // Every other line
    for( int y = 0; y < m_height; y += 2 )
    {
        // Read left and right
        for( int dx = 0; dx < 2; dx++ )
        {
            int x = 0;
            if( dx == 1 )
            {
                x = m_width - 1;
            }
            
            SimType simType = cSimType_None;
            SimPower powerOut = cSimPower_LowEdge;
            GetSimType( x, y, simType, powerOut );
            
            if( simType == cSimType_WireType0 ||
                simType == cSimType_WireType1 ||
                simType == cSimType_JumpJoint )
            {
                if( dx == 0 )
                {
                    m_inputIndices.push_back( y );
                }
                else
                {
                    m_outputIndices.push_back( y );
                }
            }
        }
    }
    
    // TODO: Initialize all not-gates...
}

WireSim::~WireSim()
{
    // ...
}

void WireSim::GetSize( int& widthOut, int& heightOut ) const
{
    widthOut = m_width;
    heightOut = m_height;
}

int WireSim::GetInputCount() const
{
    return (int)m_inputIndices.size();
}

void WireSim::SetInput( int inputIndex, bool turnOn )
{
    int pinOffset = m_inputIndices.at( inputIndex );
    
    SimType simType = cSimType_None;
    SimPower simPower = cSimPower_LowEdge;
    
    GetSimType( 0, pinOffset, simType, simPower );
    if( ( turnOn && simPower != cSimPower_HighEdge && simPower != cSimPower_RisingEdge ) ||
        ( !turnOn && simPower != cSimPower_LowEdge && simPower != cSimPower_FallingEdge ) )
    {
        SetSimType( m_image, 0, pinOffset, simType, turnOn ? cSimPower_RisingEdge : cSimPower_FallingEdge );
    }
}

int WireSim::GetOutputCount() const
{
    return (int)m_outputIndices.size();
}

void WireSim::GetOutput( int outputIndex, SimPower& powerLevel ) const
{
    int pinOffset = m_outputIndices.at( outputIndex );
    
    SimType simType = cSimType_None;
    GetSimType( m_width - 1, pinOffset, simType, powerLevel );
}

bool WireSim::Update()
{
    // Copy source to dest; update, then flip buffer
    std::vector< SimColor > resultImage = m_image;
    
    // For each pixel..
    for( int y = 0; y < m_height; y++ )
    {
        for( int x = 0; x < m_width; x++ )
        {
            Update( x, y, m_image, resultImage );
        }
    }
    
    // Check for differences
    int count = 0;
    for( int i = 0; i < (int)resultImage.size(); i++ )
    {
        if( m_image.at( i ) != resultImage.at( i ) )
        {
            count++;
        }
    }
    
    // Save to output
    m_image = resultImage;
    
    return ( count > 0 );
}

bool WireSim::GetSimType( const SimColor& givenColor, SimType& simTypeOut, SimPower& powerOut ) const
{
    // Linear search
    // Todo: could do a hash for fast lookup
    for( int i = 0; i < (int)cSimTypeCount; i++ )
    {
        for( int j = 0; j < cSimPowerCount; j++ )
        {
            // Match found!
            if( cSimColors[ i ][ j ] == givenColor )
            {
                simTypeOut = (SimType)i;
                powerOut = (SimPower)j;
                return true;
            }
        }
    }
    
    // Never found
    return false;
}

bool WireSim::GetSimType( int x, int y, SimType& simTypeOut, SimPower& powerOut ) const
{
    int linearIndex = GetLinearPosition( x, y );
    return GetSimType( m_image.at( linearIndex ), simTypeOut, powerOut );
}

bool WireSim::SaveState( const char* pngOutFileName, int pixelSize, bool highlightEdgeChanges )
{
    // Convert from ARGB to RGBA
    std::vector< unsigned char > outImage;
    outImage.resize( m_width * m_height * pixelSize * pixelSize * 4 );
    
    for( int y = 0; y < m_height; y++ )
    {
        for( int x = 0; x < m_width; x++ )
        {
            // Grab the power type
            SimType simType = cSimType_None;
            SimPower simPower = cSimPower_LowEdge;
            GetSimType( x, y, simType, simPower );

            // Grab color state directly from image buffer
            int r = ( ( cSimColors[ simType ][ simPower ] & 0x00ff0000 ) >> 16 );
            int g = ( ( cSimColors[ simType ][ simPower ] & 0x0000ff00 ) >> 8 );
            int b = (   cSimColors[ simType ][ simPower ] & 0x000000ff );
            int a = ( 0xFF );
            
            for( int dy = 0; dy < pixelSize; dy++ )
            {
                for( int dx = 0; dx < pixelSize; dx++ )
                {
                    bool drawEdge = highlightEdgeChanges &&
                                    //( simType != cSimType_None ) &&
                                    ( simPower == cSimPower_RisingEdge || simPower == cSimPower_FallingEdge ) &&
                                    ( dx == 0 || dx == ( pixelSize - 1 ) || dy == 0 || dy == ( pixelSize - 1 ) );

                    int index = ( y * pixelSize + dy ) * m_width * pixelSize * 4 + ( x * pixelSize + dx ) * 4;
                    outImage.at( index + 0 ) = drawEdge ? 0x00 : r;
                    outImage.at( index + 1 ) = drawEdge ? 0x00 : g;
                    outImage.at( index + 2 ) = drawEdge ? 0x00 : b;
                    outImage.at( index + 3 ) = a;
                }
            }
        }
    }
    
    // Write out
    unsigned int error = lodepng::encode( pngOutFileName, outImage, m_width * pixelSize, m_height * pixelSize );
    if( error != 0 )
    {
        printf( "Error encoding\n" );
        return false;
    }
    
    return true;
}

inline bool WireSim::IsBounded( int x, int y ) const
{
    if( x < 0 || y < 0 || x >= m_width || y >= m_height )
    {
        return false;
    }
    else
    {
        return true;
    }
}

inline int WireSim::GetLinearPosition( int x, int y ) const
{
    return y * m_width + x;
}

void WireSim::Update( int x, int y, const std::vector< SimColor >& source, std::vector< SimColor >& dest )
{
    // Get the 3x3 grid centered on the target
    SimType nodeGrid[ 3 ][ 3 ] = {
        { cSimType_None, cSimType_None, cSimType_None },
        { cSimType_None, cSimType_None, cSimType_None },
        { cSimType_None, cSimType_None, cSimType_None },
    };
    
    // Get the 3x3 power levels
    SimPower powerGrid[ 3 ][ 3 ] = {
        { cSimPower_LowEdge, cSimPower_LowEdge, cSimPower_LowEdge },
        { cSimPower_LowEdge, cSimPower_LowEdge, cSimPower_LowEdge },
        { cSimPower_LowEdge, cSimPower_LowEdge, cSimPower_LowEdge },
    };
    
    for( int dx = -1; dx <= 1; dx++ )
    {
        for( int dy = -1; dy <= 1; dy++ )
        {
            if( IsBounded( x + dx, y + dy ) )
            {
                GetSimType( x + dx, y + dy, nodeGrid[ dx + 1 ][ dy + 1 ], powerGrid[ dx + 1 ][ dy + 1 ] );
            }
        }
    }
    
    // Alias for tile we're simulating
    const SimType& centerType = nodeGrid[ 1 ][ 1 ];
    SimPower centerPower = powerGrid[ 1 ][ 1 ];
    
    // Ignore if undefined simulation tile
    if( centerType == cSimType_None )
    {
        return;
    }
    
    // Settle the power state on this node
    bool powerChanged = false;
    if( centerPower == cSimPower_RisingEdge )
    {
        powerChanged = true;
        centerPower = cSimPower_HighEdge;
        SetSimType( dest, x, y, centerType, cSimPower_HighEdge );
    }
    else if( centerPower == cSimPower_FallingEdge )
    {
        powerChanged = true;
        centerPower = cSimPower_LowEdge;
        SetSimType( dest, x, y, centerType, cSimPower_LowEdge );
    }
    
    // Check with type
    switch( centerType )
    {
        // Wire spreads directly adjacent to no-power nodes if they are
        // the same wire type or a joint
        case cSimType_WireType0:
        case cSimType_WireType1:
            {
                // Only propogate on edge-change
                if( powerChanged )
                {
                    // For each directly-adjacent element..
                    for( int i = 0; i < cDirectlyAdjacentOffsetCount; i++ )
                    {
                        const Vec2& adjacentOffset = cDirectlyAdjacentOffsets[ i ];
                        const SimPower& adjPower = powerGrid[ adjacentOffset.x ][ adjacentOffset.y ];
                        const SimType& adjType = nodeGrid[ adjacentOffset.x ][ adjacentOffset.y ];
                    
                        // Only spread to other directly-connected same-type wires
                        if( IsWire( adjType ) && IsSettled( adjPower ) && centerPower != adjPower  )
                        {
                            SetSimType( dest, x + adjacentOffset.x - 1, y + adjacentOffset.y - 1, adjType, ( centerPower == cSimPower_HighEdge ) ? cSimPower_RisingEdge : cSimPower_FallingEdge );
                        }
                    }
                }
            }
            
            break;
            
        case cSimType_JumpJoint:
        case cSimType_NotGate:
            {
                // Not-gates are and jump-gates are the only directional gates that only move either
                // top-down/left-right OR down-top/right-left based on their off/on state, e.g. a light purple means it is on,
                // which means it is a not-gate that operates down-top/right-left, while a dark brown means it is off, so the gate
                // operators on top-down/left-right. Also note that state-transfer only happens on settled inputs and outputs
                int powerOffset = ( centerPower == cSimPower_LowEdge ) ? 2 : 0;
                for( int i = 0; i < 2; i++ )
                {
                    int sourceIndex = powerOffset + i;
                    const Vec2& sourceOffset = cDirectlyAdjacentOffsets[ sourceIndex ];

                    // Get input details
                    const SimPower& sourcePower = powerGrid[ sourceOffset.x ][ sourceOffset.y ];
                    const SimType& sourceType = nodeGrid[ sourceOffset.x ][ sourceOffset.y ];

                    // Save input state; ignore if not settled
                    SimPower resultPower = sourcePower;
                    if( !IsWire( sourceType ) || !IsSettled( sourcePower ) )
                    {
                        continue;
                    }

                    // Flip if not-gate
                    if( centerType == cSimType_NotGate )
                    {
                        resultPower = ( resultPower == cSimPower_LowEdge ) ? cSimPower_HighEdge : cSimPower_LowEdge;
                    }

                    // Only apply onto directly opposite edges
                    const Vec2& outputPos = cDirectlyAdjacentOffsets[ ( sourceIndex + 2 ) % cDirectlyAdjacentOffsetCount ];
                    
                    // Get type and power
                    const SimType& outputType = nodeGrid[ outputPos.x ][ outputPos.y ];
                    const SimPower& outputPower = powerGrid[ outputPos.x ][ outputPos.y ];
                    
                    // Set output state only if there is a difference
                    if( IsWire( outputType ) && IsSettled( outputPower ) && outputPower != resultPower )
                    {
                        SimPower newEdgePower = ( resultPower == cSimPower_LowEdge ) ? cSimPower_FallingEdge : cSimPower_RisingEdge;
                        SetSimType( dest, x + outputPos.x - 1, y + outputPos.y - 1, outputType, newEdgePower );
                    }
                }
            }
            break;

        case cSimType_AndGate:
        case cSimType_OrGate:
        case cSimType_XorGate:
            {
                // Total on and off; we ignore non-wire corner tiles
                int onCount = 0;
                int offCount = 0;
                
                // For each input (corner wires)
                for( int i = 0; i < cCornerOffsetCount; i++ )
                {
                    const Vec2& inputPos = cCornerOffsets[ i ];
                    
                    // Get type and power
                    const SimType& inputType = nodeGrid[ inputPos.x ][ inputPos.y ];
                    const SimPower& inputPower = powerGrid[ inputPos.x ][ inputPos.y ];
                    
                    // Save input state
                    if( IsWire( inputType ) && IsSettled( inputPower ) )
                    {
                        onCount += ( inputPower == cSimPower_HighEdge ) ? 1 : 0;
                        offCount += ( inputPower == cSimPower_LowEdge ) ? 1 : 0;
                    }
                }
                
                
                // Compute and place result; break out
                SimPower resultPower = cSimPower_LowEdge;
                
                if( centerType == cSimType_AndGate && onCount >= 2 && offCount <= 0 )
                {
                    resultPower = cSimPower_HighEdge;
                }
                else if( centerType == cSimType_OrGate && onCount >= 1 && offCount > 0 )
                {
                    resultPower = cSimPower_HighEdge;
                }
                else if( centerType == cSimType_XorGate && onCount == 1 && offCount > 0 )
                {
                    resultPower = cSimPower_HighEdge;
                }
                
                // Only apply the edge on settled states
                for( int i = 0; i < cDirectlyAdjacentOffsetCount; i++ )
                {
                    const Vec2& outputPos = cDirectlyAdjacentOffsets[ i ];
                    
                    // Get type and power
                    const SimType& outputType = nodeGrid[ outputPos.x ][ outputPos.y ];
                    const SimPower& outputPower = powerGrid[ outputPos.x ][ outputPos.y ];
                    
                    // Set input state only if there is a state change
                    if( IsWire( outputType ) && IsSettled( outputPower ) && outputPower != resultPower )
                    {
                        SetSimType( dest, x + outputPos.x - 1, y + outputPos.y - 1, outputType, ( outputPower == cSimPower_HighEdge ) ? cSimPower_RisingEdge : cSimPower_FallingEdge );
                    }
                }
            }
            
            break;
            
            // Ignore these cases
        case cSimType_None:
        default:
            break;
    }
}

void WireSim::SetSimType( std::vector< SimColor >& dstImage, int x, int y, const SimType& simType, SimPower powerLevel )
{
    int linearIndex = GetLinearPosition( x, y );
    dstImage.at( linearIndex ) = MakeSimColor( simType, powerLevel );
}

WireSim::SimColor WireSim::MakeSimColor( const SimType& simType, SimPower powerLevel )
{
    return cSimColors[ simType ][ powerLevel ];
}

bool WireSim::IsEdge( const SimPower& simPower ) const
{
    return ( simPower == cSimPower_FallingEdge || simPower == cSimPower_RisingEdge );
}

bool WireSim::IsSettled( const SimPower& simPower ) const
{
    return ( simPower == cSimPower_LowEdge || simPower == cSimPower_HighEdge );
}

bool WireSim::IsWire( const SimType& simType ) const
{
    return ( simType == cSimType_WireType0 || simType == cSimType_WireType1 );
}

bool WireSim::IsGate( const SimType& simType ) const
{
    return ( simType == cSimType_JumpJoint ||
             simType == cSimType_AndGate ||
             simType == cSimType_OrGate ||
             simType == cSimType_XorGate ||
             simType == cSimType_NotGate
            );
}
