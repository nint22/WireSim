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
    // Based on Tango Icon Theme; four levels of power color
    // No power, spreading, power, spreading
    const WireSim::SimColor cSimColors[ WireSim::SimTypeCount ][ WireSim::SimPowerCount ] =
    {
        { 0x00000000, 0x00000000, 0x00000000, 0x00000000 }, // None-type
        { 0x00fcaf3e, 0x00fcaf4f, 0x00f57900, 0x00f57911 }, // Wire 0 (Orange)
        { 0x008ae234, 0x008ae245, 0x004e9a06, 0x004e9a17 }, // Wire 1 (Green)
        { 0x00e9b96e, 0x00e9b96f, 0x008f5902, 0x008f5903 }, // Jump Joint (Brown)
        { 0x00fce94f, 0x00fce950, 0x00c4a000, 0x00c4a001 }, // Merge joint (Yellow)
        { 0x00ef2929, 0x00ef292a, 0x00a40000, 0x00a40001 }, // And (Red)
        { 0x00729fcf, 0x00729fd0, 0x00204a87, 0x00204a88 }, // Or (Blue)
        { 0x00888a85, 0x00888a86, 0x00484845, 0x00484846 }, // Xor (Grey)
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
    
    // All adjacent (corners included)
    const int cAdjacentOffsetCount = 8;
    const Vec2 cAdjacentOffsets[ cAdjacentOffsetCount ] =
    {
        Vec2( 2, 1 ),
        Vec2( 1, 2 ),
        Vec2( 0, 1 ),
        Vec2( 1, 0 ),
        Vec2( 0, 0 ),
        Vec2( 0, 2 ),
        Vec2( 2, 0 ),
        Vec2( 2, 2 ),
    };
    
    // All combinations of two-input one output, Y-shape
    // 4 orientations (top-down, left-right, down-up, right-left), listed { input a, input b, output }[x, y]
    const int cTernaryOffsetCount = 4;
    const Vec2 cTernaryOffsets[ cTernaryOffsetCount ][ 3 ] =
    {
        { // Top-down
            Vec2( 0, 0 ),
            Vec2( 2, 0 ),
            Vec2( 1, 2 ),
        },
        { // Left-right
            Vec2( 0, 0 ),
            Vec2( 0, 2 ),
            Vec2( 2, 1 ),
        },
        { // Down-up
            Vec2( 0, 2 ),
            Vec2( 2, 2 ),
            Vec2( 1, 0 ),
        },
        { // Right-left
            Vec2( 2, 0 ),
            Vec2( 2, 2 ),
            Vec2( 0, 1 ),
        },
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
            
            SimType simType = SimType_None;
            SimPower powerOut = SimPower_LowEdge;
            GetSimType( x, y, simType, powerOut );
            
            if( simType == SimType_WireType0 ||
                simType == SimType_WireType1 ||
                simType == SimType_MergeJoint ||
                simType == SimType_JumpJoint )
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
}

WireSim::~WireSim()
{
    // Todo: load data into color buffer
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
    
    SimType simType = SimType_None;
    SimPower simPower = SimPower_LowEdge;
    
    GetSimType( 0, pinOffset, simType, simPower );
    if( ( turnOn && simPower != SimPower_HighEdge && simPower != SimPower_RisingEdge ) ||
        ( !turnOn && simPower != SimPower_LowEdge && simPower != SimPower_FallingEdge ) )
    {
        SetSimType( m_image, 0, pinOffset, simType, turnOn ? SimPower_RisingEdge : SimPower_FallingEdge );
    }
}

int WireSim::GetOutputCount() const
{
    return (int)m_outputIndices.size();
}

void WireSim::GetOutput( int outputIndex, SimPower& powerLevel ) const
{
    int pinOffset = m_outputIndices.at( outputIndex );
    
    SimType simType = SimType_None;
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
    for( int i = 0; i < (int)SimTypeCount; i++ )
    {
        for( int j = 0; j < SimPowerCount; j++ )
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
            SimType simType;
            SimPower simPower;
            GetSimType( x, y, simType, simPower );

            // Grab color state directly from image buffer
            int r = ( ( m_image.at( y * m_width + x ) & 0x00ff0000 ) >> 16 );
            int g = ( ( m_image.at( y * m_width + x ) & 0x0000ff00 ) >> 8 );
            int b = (   m_image.at( y * m_width + x ) & 0x000000ff );
            int a = ( 0xFF );
            
            for( int dy = 0; dy < pixelSize; dy++ )
            {
                for( int dx = 0; dx < pixelSize; dx++ )
                {
                    bool drawEdge = highlightEdgeChanges &&
                                    ( simPower == SimPower_RisingEdge || simPower == SimPower_FallingEdge ) &&
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
    // TODO: These state change checks are pretty complicated, and can raise some issues of timing.. Look at how
    // complicated the merge-joint is: what if we simplified each gate in not having a state, but the different
    // colors defining their orientation: a light-purple tile is a top-down not-gate, while a dark-purple is a left-right gate.
    // This makes the sim simpler, but forces us to design structures left-right rather than in any direction. Wires do have
    // their state defined through color, and have the concept of a wave (edge-rising, and edge-risen, etc.), so that way if we
    // need more space, we can just move the wiring around as appropriate
    // TODO: Circuit initialization system? Not-gates have to be initialized somehow... e.g. when input is off, they have to send a rise-signal? Maybe it's an initialization thing?
    // TODO: Support a test-input text file, where you can define which pins to raise / lower, for how many cycles, and write out the output in some sort of easy-to-read text file

    // Get the 3x3 grid centered on the target
    SimType nodeGrid[ 3 ][ 3 ] = {
        { SimType_None, SimType_None, SimType_None },
        { SimType_None, SimType_None, SimType_None },
        { SimType_None, SimType_None, SimType_None },
    };
    
    // Get the 3x3 power levels
    SimPower powerGrid[ 3 ][ 3 ] = {
        { SimPower_LowEdge, SimPower_LowEdge, SimPower_LowEdge },
        { SimPower_LowEdge, SimPower_LowEdge, SimPower_LowEdge },
        { SimPower_LowEdge, SimPower_LowEdge, SimPower_LowEdge },
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
    if( centerType == SimType_None )
    {
        return;
    }
    
    // Settle the power state on this node
    if( centerPower == SimPower_RisingEdge )
    {
        centerPower = SimPower_HighEdge;
        SetSimType( dest, x, y, centerType, SimPower_HighEdge );
    }
    else if( centerPower == SimPower_FallingEdge )
    {
        centerPower = SimPower_LowEdge;
        SetSimType( dest, x, y, centerType, SimPower_LowEdge );
    }
    
    // Check with type
    switch( centerType )
    {
        // Wire spreads directly adjacent to no-power nodes if they are
        // the same wire type or a joint
        case SimType_WireType0:
        case SimType_WireType1:
            {
                // For each directly-adjacent element, test if on
                for( int i = 0; i < cDirectlyAdjacentOffsetCount; i++ )
                {
                    const Vec2& adjacentOffset = cDirectlyAdjacentOffsets[ i ];
                    const SimPower& adjPower = powerGrid[ adjacentOffset.x ][ adjacentOffset.y ];
                    const SimType& adjType = nodeGrid[ adjacentOffset.x ][ adjacentOffset.y ];
                
                    // Don't spread
                    if( adjType != SimType_None )
                    {
                        if( centerPower == SimPower_LowEdge && ( adjPower == SimPower_HighEdge || adjPower == SimPower_RisingEdge ) )
                        {
                            SetSimType( dest, x, y, centerType, SimPower_RisingEdge );
                        }
                        else if( centerPower == SimPower_HighEdge && adjPower == SimPower_FallingEdge )
                        {
                            SetSimType( dest, x, y, centerType, SimPower_LowEdge );
                        }
                    }
                }
            }
            
            break;
            
        case SimType_JumpJoint:
            {
                // Check source-to-dest connections, starting right-to-left, then bottom-to-up, left-to-right, then top-to-bottom
                // Only applies it if the source is high and dest is low
                for( int i = 0; i < cDirectlyAdjacentOffsetCount; i++ )
                {
                    // Source
                    const Vec2& srcOffset = cDirectlyAdjacentOffsets[ i ];

                    const SimPower& srcPower = powerGrid[ srcOffset.x ][ srcOffset.y ];
                    const SimType& srcType = nodeGrid[ srcOffset.x ][ srcOffset.y ];

                    // Ignore if source isn't a type or isn't spreading
                    if( srcType == SimType_None || ( srcPower != SimPower_RisingEdge && srcPower != SimPower_FallingEdge ) )
                    {
                        continue;
                    }

                    const Vec2& dstOffset = cDirectlyAdjacentOffsets[ ( i + 2 ) % cDirectlyAdjacentOffsetCount ];

                    const SimPower& dstPower = powerGrid[ dstOffset.x ][ dstOffset.y ];
                    const SimType& dstType = nodeGrid[ dstOffset.x ][ dstOffset.y ];

                    // Ignore if destination isn't a type or it's another jump joint, or ignore if the source has the same power state as the dest
                    if( dstType == SimType_None ||
                        dstType == SimType_JumpJoint ||
                        ( srcPower == SimPower_HighEdge && ( dstPower == SimPower_HighEdge || dstPower == SimPower_RisingEdge ) ) ||
                        ( srcPower == SimPower_LowEdge && ( dstPower == SimPower_LowEdge || dstPower == SimPower_FallingEdge ) ) )
                    {
                        continue;
                    }

                    if( srcPower == SimPower_RisingEdge )
                    {
                        SetSimType( dest, x + dstOffset.x - 1, y + dstOffset.y - 1, dstType, SimPower_RisingEdge );
                    }
                    else
                    {
                        SetSimType( dest, x + dstOffset.x - 1, y + dstOffset.y - 1, dstType, SimPower_FallingEdge );
                    }
                }
            }
            break;
            
        case SimType_MergeJoint:
            {
                // For each directly adjacent tile, check for edge-rises then edge-falls; distribute this state
                // to all stable directly adjacent tiles
                int edgeRiseIndex = -1;
                int edgeFallIndex = -1;

                for( int i = 0; i < cDirectlyAdjacentOffsetCount; i++ )
                {
                    // Source
                    const Vec2& srcOffset = cDirectlyAdjacentOffsets[ i ];
                
                    const SimPower& srcPower = powerGrid[ srcOffset.x ][ srcOffset.y ];
                    const SimType& srcType = nodeGrid[ srcOffset.x ][ srcOffset.y ];
                
                    // Ignore if destination isn't a type or it's another jump joint, or ignore if the source has the same power state as the dest
                    if( srcType != SimType_None )
                    {
                        if( srcPower == SimPower_RisingEdge )
                        {
                            edgeRiseIndex = i;
                            break;
                        }
                        else if( srcPower == SimPower_FallingEdge )
                        {
                            edgeRiseIndex = i;
                            break;
                        }
                    }
                }
            
                // Turn on or off all other edges..
                for( int i = 0; i < cDirectlyAdjacentOffsetCount; i++ )
                {
                    // Source
                    const Vec2& dstOffset = cDirectlyAdjacentOffsets[ i ];

                    const SimPower& dstPower = powerGrid[ dstOffset.x ][ dstOffset.y ];
                    const SimType& dstType = nodeGrid[ dstOffset.x ][ dstOffset.y ];

                    if( dstType != SimType_None )
                    {
                        if( edgeRiseIndex >= 0 && dstPower == SimPower_LowEdge )
                        {
                            SetSimType( dest, x + dstOffset.x - 1, y + dstOffset.y - 1, dstType, SimPower_RisingEdge );
                        }
                        else if( edgeFallIndex >= 0 && dstPower == SimPower_HighEdge )
                        {
                            SetSimType( dest, x + dstOffset.x - 1, y + dstOffset.y - 1, dstType, SimPower_FallingEdge );
                        }
                    }
                }
            }
            break;
            
        case SimType_AndGate:
        case SimType_OrGate:
        case SimType_XorGate:
            {
                // Test top-and-bottom for inputs, then send it right-left, else test otherwise
                for( int i = 0; i < cTernaryOffsetCount; i++ )
                {
                    const Vec2& inputPosA = cTernaryOffsets[ i ][ 0 ];
                    const Vec2& inputPosB = cTernaryOffsets[ i ][ 1 ];
                    const Vec2& outputPos = cTernaryOffsets[ i ][ 2 ];

                    // Get type and power
                    const SimPower& powerA = powerGrid[ inputPosA.x ][ inputPosA.y ];
                    const SimType& typeA = nodeGrid[ inputPosA.x ][ inputPosA.y ];

                    const SimPower& powerB = powerGrid[ inputPosB.x ][ inputPosB.y ];
                    const SimType& typeB = nodeGrid[ inputPosB.x ][ inputPosB.y ];

                    const SimPower& powerOut = powerGrid[ outputPos.x ][ outputPos.y ];
                    const SimType& typeOut = nodeGrid[ outputPos.x ][ outputPos.y ];

                    // Inputs & outputs must be not-none and be settled; output just has to be a valid type
                    if( typeA == SimType_None || ( powerA != SimPower_LowEdge && powerA != SimPower_HighEdge ) ||
                        typeB == SimType_None || ( powerB != SimPower_LowEdge && powerB != SimPower_HighEdge ) ||
                        typeOut == SimType_None )
                    {
                        continue;
                    }

                    // Compute and place result; break out
                    SimPower resultPower = SimPower_LowEdge;

                    if( centerType == SimType_AndGate )
                    {
                        resultPower = ( powerA == SimPower_HighEdge && powerB == SimPower_HighEdge ) ? SimPower_HighEdge : SimPower_LowEdge;
                    }
                    else if( centerType == SimType_OrGate )
                    {
                        resultPower = ( powerA == SimPower_HighEdge || powerB == SimPower_HighEdge ) ? SimPower_HighEdge : SimPower_LowEdge;
                    }
                    else if( centerType == SimType_XorGate )
                    {
                        resultPower = ( powerA != powerB ) ? SimPower_HighEdge : SimPower_LowEdge;
                    }

                    // If there is now going to be a state change, change the power to be rising or falling
                    if( resultPower != powerOut )
                    {
                        resultPower = resultPower == SimPower_HighEdge ? SimPower_RisingEdge : SimPower_FallingEdge;
                    }

                    // Apply
                    SetSimType( dest, x + outputPos.x - 1, y + outputPos.y - 1, typeOut, resultPower );
                }
            }
            
            break;

        case SimType_NotGate:
            {
                // Check source-to-dest connections, starting right-to-left, then bottom-to-up, left-to-right, then top-to-bottom
                // Only applies it if the source is rising/falling and dest is stable; once a state is found, bail out
                for( int i = 0; i < cDirectlyAdjacentOffsetCount; i++ )
                {
                    // Source
                    const Vec2& srcOffset = cDirectlyAdjacentOffsets[ i ];

                    const SimPower& srcPower = powerGrid[ srcOffset.x ][ srcOffset.y ];
                    const SimType& srcType = nodeGrid[ srcOffset.x ][ srcOffset.y ];

                    // Ignore if source isn't a type or isn't spreading
                    if( srcType == SimType_None || ( srcPower != SimPower_RisingEdge && srcPower != SimPower_FallingEdge ) )
                    {
                        continue;
                    }

                    const Vec2& dstOffset = cDirectlyAdjacentOffsets[ ( i + 2 ) % cDirectlyAdjacentOffsetCount ];

                    const SimPower& dstPower = powerGrid[ dstOffset.x ][ dstOffset.y ];
                    const SimType& dstType = nodeGrid[ dstOffset.x ][ dstOffset.y ];

                    // Ignore if destination isn't a type or it's another jump joint, or ignore if the source has the same power state as the dest
                    if( dstType == SimType_None ||
                        dstType == SimType_JumpJoint ||
                        ( srcPower == SimPower_HighEdge && ( dstPower == SimPower_HighEdge || dstPower == SimPower_RisingEdge ) ) ||
                        ( srcPower == SimPower_LowEdge && ( dstPower == SimPower_LowEdge || dstPower == SimPower_FallingEdge ) ) )
                    {
                        continue;
                    }

                    if( srcPower == SimPower_RisingEdge )
                    {
                        SetSimType( dest, x + dstOffset.x - 1, y + dstOffset.y - 1, dstType, SimPower_FallingEdge );
                    }
                    else
                    {
                        SetSimType( dest, x + dstOffset.x - 1, y + dstOffset.y - 1, dstType, SimPower_RisingEdge );
                    }
                }
            }
            break;
            
            // Ignore these cases
        case SimType_None:
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
