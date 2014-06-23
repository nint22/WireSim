/***

 WireSim: Esoteric Computer built out of discrete gates, wholly simulated with hand-made images, execute through graphics cards!
 
***/

#include <vector>

#include "lodepng.h"
#include "WireSim.h"

namespace
{
    // Color constants; ARGB format
    // Based on Tango Icon Theme; four levels of power color
    const WireSim::SimColor cSimColors[ WireSim::SimTypeCount ][ 4 ] =
    {
        // Least power to most power
        { 0x00000000, 0x00000000, 0x00000000, 0x00000000 }, // None-type
        { 0x00fcaf3e, 0x00f57900, 0x00ce5c00, 0x00c05c00 }, // Wire 0 (Orange)
        { 0x008ae234, 0x0073d216, 0x004e9a06, 0x00355d04 }, // Wire 1 (Green)
        { 0x00e9b96e, 0x00c17d11, 0x008f5902, 0x00704502 }, // Jump Joint (Brown)
        { 0x00fce94f, 0x00edd400, 0x00c4a000, 0x00a78900 }, // Merge joint (Yellow)
        { 0x00ef2929, 0x00cc0000, 0x00a40000, 0x00700000 }, // And (Red)
        { 0x00729fcf, 0x003465a4, 0x00204a87, 0x0015325a }, // Or (Blue)
        { 0x00888a85, 0x00636561, 0x00484845, 0x003a3a38 }, // Xor (Grey)
        { 0x00ad7fa8, 0x0075507b, 0x005c3566, 0x00482950 }, // Nor (Purple)
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
    
    // Look at the left and right edges for inputs and outputs; just wire-type colors
    for( int i = 0; i < m_height; i++ )
    {
        // Iterate top-down, left comumn then right column
        int pixelx = 0;
        if( i >= ( m_height / 2 ) )
        {
            pixelx = m_width - 1;
        }
        int pixely = (i * 2) % m_height;
        
        SimType simType = SimType_None;
        int powerOut = 0;
        GetSimType( pixelx, pixely, simType, powerOut );
        
        if( simType == SimType_WireColor0
         || simType == SimType_WireColor1
         || simType == SimType_MergeJoint
         || simType == SimType_JumpJoint )
        {
            if( pixelx == 0 )
            {
                m_inputIndices.push_back( pixely );
            }
            else
            {
                m_outputIndices.push_back( pixely );
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

void WireSim::SetInput( int inputIndex, int powerLevel )
{
    int pinOffset = m_inputIndices.at( inputIndex );
    
    SimType simType = SimType_None;
    int unusedPower = 0;
    
    GetSimType( 0, pinOffset, simType, unusedPower );
    SetSimType( m_image, 0, pinOffset, simType, powerLevel );
}

int WireSim::GetOutputCount() const
{
    return (int)m_outputIndices.size();
}

void WireSim::GetOutput( int outputIndex, int& powerLevel ) const
{
    int pinOffset = m_outputIndices.at( outputIndex );
    
    SimType simType = SimType_None;
    GetSimType( m_width - 1, pinOffset, simType, powerLevel );
}

void WireSim::Update()
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
    /*
    int count = 0;
    for( int i = 0; i < (int)resultImage.size(); i++ )
    {
        if( m_image.at( i ) != resultImage.at( i ) )
        {
            count++;
        }
    }
    */
    
    // Save to output
    m_image = resultImage;
}

bool WireSim::GetSimType( const SimColor& givenColor, SimType& simTypeOut, int& powerOut ) const
{
    // Linear search
    // Todo: could do a hash for fast lookup
    for( int i = 0; i < (int)SimTypeCount; i++ )
    {
        for( int j = 0; j < 4; j++ )
        {
            // Match found!
            if( cSimColors[ i ][ j ] == givenColor )
            {
                simTypeOut = (SimType)i;
                powerOut = j;
                return true;
            }
        }
    }

    // Never found
    return false;
}

bool WireSim::GetSimType( int x, int y, SimType& simTypeOut, int& powerOut ) const
{
    int linearIndex = GetLinearPosition( x, y );
    return GetSimType( m_image.at( linearIndex ), simTypeOut, powerOut );
}

bool WireSim::SaveState( const char* pngOutFileName )
{
    // Convert from ARGB to RGBA
    std::vector< unsigned char > outImage;
    for( int i = 0; i < m_image.size(); i++ )
    {
        outImage.push_back( ( m_image.at( i ) & 0x00ff0000 ) >> 16 );
        outImage.push_back( ( m_image.at( i ) & 0x0000ff00 ) >> 8 );
        outImage.push_back(   m_image.at( i ) & 0x000000ff );
        outImage.push_back( 0xFF );
    }
    
    // Write out
    unsigned int error = lodepng::encode( pngOutFileName, outImage, m_width, m_height );
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
    /*** Position Iterators ***/
    
    // Directly adjacent offsets
    static const int cDirectlyAdjacentOffsets[ 4 ][ 2 ] = {
        { 2, 1 },
        { 1, 2 },
        { 0, 1 },
        { 1, 0 },
    };
    
    // All adjacent (corners included)
    static const int cAdjacentOffsets[ 8 ][ 2 ] = {
        { 2, 1 },
        { 1, 2 },
        { 0, 1 },
        { 1, 0 },
        { 0, 0 },
        { 0, 2 },
        { 2, 0 },
        { 2, 2 },
    };
    
    // All combinations of two-input one output, Y-shape
    // 4 orientations (top-down, left-right, down-up, right-left), listed { input a, input b, output }[x, y]
    static const int cTernaryOffsets[ 4 ][ 3 ][ 2 ] =
    {
        { // Top-down
            { 0, 0 },
            { 2, 0 },
            { 1, 2 },
        },
        { // Left-right
            { 0, 0 },
            { 0, 2 },
            { 2, 1 },
        },
        { // Down-up
            { 0, 2 },
            { 2, 2 },
            { 1, 0 },
        },
        { // Right-left
            { 2, 0 },
            { 2, 2 },
            { 0, 1 },
        },
    };
    
    /*** Cell Simulation ***/
    
    // Get the 3x3 grid centered on the target
    SimType nodeGrid[ 3 ][ 3 ] = {
        { SimType_None, SimType_None, SimType_None },
        { SimType_None, SimType_None, SimType_None },
        { SimType_None, SimType_None, SimType_None },
    };
    
    // Get the 3x3 power levels
    int powerGrid[ 3 ][ 3 ] = {
        { 0, 0, 0 },
        { 0, 0, 0 },
        { 0, 0, 0 },
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
    
    // Each node loses power, but gets simulated if highest power
    int powerLevel = powerGrid[ 1 ][ 1 ];
    if( powerLevel <= 0 )
    {
        return;
    }
    
    SimType centerType = nodeGrid[ 1 ][ 1 ];
    if( centerType != SimType_None )
    {
        SetSimType( dest, x, y, centerType, powerLevel - 1 );
    }
    
    // Only continue simulation if 3
    if( powerLevel != 3 )
    {
        return;
    }
    
    // Check with type
    switch( centerType )
    {
        // Wire spreads directly adjacent to no-power nodes if they are
        // the same wire type or a joint
        case SimType_WireColor0:
        case SimType_WireColor1:
            
            // For each directly-adjacent wire and joint
            for( int i = 0; i < 4; i++ )
            {
                int index[ 2 ] =
                {
                    cDirectlyAdjacentOffsets[ i ][ 0 ],
                    cDirectlyAdjacentOffsets[ i ][ 1 ],
                };
                
                SimType adjSimType = nodeGrid[ index[ 0 ] ][ index[ 1 ] ];
                int powerLevel = powerGrid[ index[ 0 ] ][ index[ 1 ] ];
                
                if( ( powerLevel == 0 ) && ( adjSimType == centerType || adjSimType == SimType_JumpJoint || adjSimType == SimType_MergeJoint ) )
                {
                    SetSimType( dest, x + index[ 0 ] - 1, y + index[ 1 ] - 1, adjSimType, 3 ); // 3 is max power
                }
            }
            
            break;

        case SimType_JumpJoint:
            
            // For each directly-adjacent to wires only
            for( int i = 0; i < 4; i++ )
            {
                int index[ 2 ] =
                {
                    cDirectlyAdjacentOffsets[ i ][ 0 ],
                    cDirectlyAdjacentOffsets[ i ][ 1 ],
                };
                
                SimType adjSimType = nodeGrid[ index[ 0 ] ][ index[ 1 ] ];
                int powerLevel = powerGrid[ index[ 0 ] ][ index[ 1 ] ];
                
                if( ( powerLevel == 0 )
                 && ( adjSimType != SimType_None ) )
                {
                    SetSimType( dest, x + index[ 0 ] - 1, y + index[ 1 ] - 1, adjSimType, 3 ); // 3 is max power
                }
            }
            
            break;

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
                int powerLevel = powerGrid[ index[ 0 ] ][ index[ 1 ] ];
                
                if( ( powerLevel == 0 )
                 && ( adjSimType != SimType_None ) )
                {
                    SetSimType( dest, x + index[ 0 ] - 1, y + index[ 1 ] - 1, adjSimType, 3 ); // 3 is max power
                }
            }
            
            break;

        case SimType_AndGate:
            
            // For each teranary gate configuration
            for( int i = 0; i < 4; i++ )
            {
                // Left to right
                //SimType inputTypeA = nodeGrid[ cTernaryOffsets[ i ][ 0 ][ 0 ] ][ cTernaryOffsets[ i ][ 0 ][ 1 ] ];
                //SimType inputTypeB = nodeGrid[ cTernaryOffsets[ i ][ 1 ][ 0 ] ][ cTernaryOffsets[ i ][ 1 ][ 1 ] ];
                SimType outputType = nodeGrid[ cTernaryOffsets[ i ][ 2 ][ 0 ] ][ cTernaryOffsets[ i ][ 2 ][ 1 ] ];
                
                int inputPowerA = powerGrid[ cTernaryOffsets[ i ][ 0 ][ 0 ] ][ cTernaryOffsets[ i ][ 0 ][ 1 ] ];
                int inputPowerB = powerGrid[ cTernaryOffsets[ i ][ 1 ][ 0 ] ][ cTernaryOffsets[ i ][ 1 ][ 1 ] ];
                int outputPower = powerGrid[ cTernaryOffsets[ i ][ 2 ][ 0 ] ][ cTernaryOffsets[ i ][ 2 ][ 1 ] ];
                
                if( outputPower == 0 && outputType != SimType_None && inputPowerA != 0 && inputPowerB != 0 )
                {
                    int tx = x + cTernaryOffsets[ i ][ 2 ][ 0 ] - 1;
                    int ty = y + cTernaryOffsets[ i ][ 2 ][ 1 ] - 1;
                    
                    SetSimType( dest, tx, ty, outputType, 3 ); // 3 is max power
                }
            }
            
            break;

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

void WireSim::SetSimType( std::vector< SimColor >& dstImage, int x, int y, const SimType& simType, int powerLevel )
{
    int linearIndex = GetLinearPosition( x, y );
    dstImage.at( linearIndex ) = MakeSimColor( simType, powerLevel );
}

WireSim::SimColor WireSim::MakeSimColor( const SimType& simType, int powerLevel )
{
    return cSimColors[ simType ][ powerLevel ];
}
