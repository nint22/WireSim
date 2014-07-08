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
        { 0x00ad7fa8, 0x00ad7fa9, 0x005c3566, 0x005c3567 }, // Nor (Purple)
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
    const Vec2 cAdjacentOffsets[ cAdjacentOffsetCount ][ 2 ] =
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
    const Vec2 cTernaryOffsets[ cTernaryOffsetCount ][ 3 ][ 2 ] =
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
            SimPower powerOut = SimPower_NoPower;
            GetSimType( x, y, simType, powerOut );
            
            if( simType == SimType_WireColor0
               || simType == SimType_WireColor1
               || simType == SimType_MergeJoint
               || simType == SimType_JumpJoint )
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
    SimPower simPower = SimPower_NoPower;
    
    GetSimType( 0, pinOffset, simType, simPower );
    if( ( turnOn && simPower != SimPower_Power && simPower != SimPower_PowerSpreading ) ||
        ( !turnOn && simPower != SimPower_NoPower && simPower != SimPower_NoPowerSpreading ) )
    {
        SetSimType( m_image, 0, pinOffset, simType, turnOn ? SimPower_PowerSpreading : SimPower_NoPowerSpreading );
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

bool WireSim::SaveState( const char* pngOutFileName, int pixelSize )
{
    // Convert from ARGB to RGBA
    std::vector< unsigned char > outImage;
    outImage.resize( m_width * m_height * pixelSize * pixelSize * 4 );
    
    for( int y = 0; y < m_height; y++ )
    {
        for( int x = 0; x < m_width; x++ )
        {
            int r = ( ( m_image.at( y * m_width + x ) & 0x00ff0000 ) >> 16 );
            int g = ( ( m_image.at( y * m_width + x ) & 0x0000ff00 ) >> 8 );
            int b = (   m_image.at( y * m_width + x ) & 0x000000ff );
            int a = ( 0xFF );
            
            for( int dy = 0; dy < pixelSize; dy++ )
            {
                for( int dx = 0; dx < pixelSize; dx++ )
                {
                    int index = ( y * pixelSize + dy ) * m_width * pixelSize * 4 + ( x * pixelSize + dx ) * 4;
                    outImage.at( index + 0 ) = r;
                    outImage.at( index + 1 ) = g;
                    outImage.at( index + 2 ) = b;
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
        { SimType_None, SimType_None, SimType_None },
        { SimType_None, SimType_None, SimType_None },
        { SimType_None, SimType_None, SimType_None },
    };
    
    // Get the 3x3 power levels
    SimPower powerGrid[ 3 ][ 3 ] = {
        { SimPower_NoPower, SimPower_NoPower, SimPower_NoPower },
        { SimPower_NoPower, SimPower_NoPower, SimPower_NoPower },
        { SimPower_NoPower, SimPower_NoPower, SimPower_NoPower },
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
    if( centerPower == SimPower_PowerSpreading )
    {
        centerPower = SimPower_Power;
        SetSimType( dest, x, y, centerType, SimPower_Power );
    }
    else if( centerPower == SimPower_NoPowerSpreading )
    {
        centerPower = SimPower_NoPower;
        SetSimType( dest, x, y, centerType, SimPower_NoPower );
    }
    
    // Check with type
    switch( centerType )
    {
        // Wire spreads directly adjacent to no-power nodes if they are
        // the same wire type or a joint
        case SimType_WireColor0:
        case SimType_WireColor1:
            
            // For each directly-adjacent element, test if on
            for( int i = 0; i < cDirectlyAdjacentOffsetCount; i++ )
            {
                const Vec2& adjacentOffset = cDirectlyAdjacentOffsets[ i ];
                const SimPower& adjPower = powerGrid[ adjacentOffset.x ][ adjacentOffset.y ];
                const SimType& adjType = nodeGrid[ adjacentOffset.x ][ adjacentOffset.y ];
                
                // Don't spread
                if( adjType != SimType_None )
                {
                    if( centerPower == SimPower_NoPower && ( adjPower == SimPower_Power || adjPower == SimPower_PowerSpreading ) )
                    {
                        SetSimType( dest, x, y, centerType, SimPower_PowerSpreading );
                    }
                    else if( centerPower == SimPower_Power && adjPower == SimPower_NoPowerSpreading )
                    {
                        SetSimType( dest, x, y, centerType, SimPower_NoPower );
                    }
                }
            }
            
            break;
            
        case SimType_JumpJoint:
            
            // Check source-to-dest connections, starting right-to-left, then bottom-to-up, left-to-right, then top-to-bottom
            // Only applies it if the source is high and dest is low
            for( int i = 0; i < cDirectlyAdjacentOffsetCount; i++ )
            {
                // Source
                const Vec2& srcOffset = cDirectlyAdjacentOffsets[ i ];

                const SimPower& srcPower = powerGrid[ srcOffset.x ][ srcOffset.y ];
                const SimType& srcType = nodeGrid[ srcOffset.x ][ srcOffset.y ];

                // Ignore if source isn't a type or isn't spreading
                if( srcType == SimType_None || ( srcPower != SimPower_PowerSpreading && srcPower != SimPower_NoPowerSpreading ) )
                {
                    continue;
                }

                const Vec2& dstOffset = cDirectlyAdjacentOffsets[ ( i + 2 ) % cDirectlyAdjacentOffsetCount ];

                const SimPower& dstPower = powerGrid[ dstOffset.x ][ dstOffset.y ];
                const SimType& dstType = nodeGrid[ dstOffset.x ][ dstOffset.y ];

                // Ignore if destination isn't a type or it's another jump joint, or ignore if the source has the same power state as the dest
                if( dstType == SimType_None ||
                    dstType == SimType_JumpJoint ||
                    ( srcPower == SimPower_Power && ( dstPower == SimPower_Power || dstPower == SimPower_PowerSpreading ) ) ||
                    ( srcPower == SimPower_NoPower && ( dstPower == SimPower_NoPower || dstPower == SimPower_NoPowerSpreading ) ) )
                {
                    continue;
                }

                if( srcPower == SimPower_PowerSpreading )
                {
                    SetSimType( dest, x + dstOffset.x - 1, y + dstOffset.y - 1, dstType, SimPower_PowerSpreading );
                }
                else
                {
                    SetSimType( dest, x + dstOffset.x - 1, y + dstOffset.y - 1, dstType, SimPower_NoPowerSpreading );
                }
            }
            
            break;
            /*
        case SimType_MergeJoint:
            
            // For each directly-adjacent to wires only
            for( int i = 0; i < 8; i++ )
            {
                int index[ 2 ] =
                {
                    cAdjacentOffsets[ i ][ 0 ],
                    cAdjacentOffsets[ i ][ 1 ],
                };
                
                SimType adjSimType = nodeGrid[ index[ 0 ] ][ index[ 1 ] ];
                
                if( adjSimType != SimType_None )
                {
                    SetSimType( dest, x + index[ 0 ] - 1, y + index[ 1 ] - 1, adjSimType, powerLevel );
                }
            }
            
            break;
            
        case SimType_AndGate:
            
            // For each ternary gate configuration
            for( int i = 0; i < 4; i++ )
            {
                // Left to right
                //SimType inputTypeA = nodeGrid[ cTernaryOffsets[ i ][ 0 ][ 0 ] ][ cTernaryOffsets[ i ][ 0 ][ 1 ] ];
                //SimType inputTypeB = nodeGrid[ cTernaryOffsets[ i ][ 1 ][ 0 ] ][ cTernaryOffsets[ i ][ 1 ][ 1 ] ];
                SimType outputType = nodeGrid[ cTernaryOffsets[ i ][ 2 ][ 0 ] ][ cTernaryOffsets[ i ][ 2 ][ 1 ] ];
                
                SimPower inputPowerA = powerGrid[ cTernaryOffsets[ i ][ 0 ][ 0 ] ][ cTernaryOffsets[ i ][ 0 ][ 1 ] ];
                SimPower inputPowerB = powerGrid[ cTernaryOffsets[ i ][ 1 ][ 0 ] ][ cTernaryOffsets[ i ][ 1 ][ 1 ] ];
                
                if( ( outputType != SimType_None ) && ( inputPowerA == SimPower_Power && inputPowerB == SimPower_Power ) )
                {
                    int tx = x + cTernaryOffsets[ i ][ 2 ][ 0 ] - 1;
                    int ty = y + cTernaryOffsets[ i ][ 2 ][ 1 ] - 1;
                    
                    SetSimType( dest, tx, ty, outputType, powerLevel );
                }
            }
            
            break;
            */
        case SimType_OrGate:
            break;
            
        case SimType_XorGate:
            break;
            
        case SimType_NotGate:
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
