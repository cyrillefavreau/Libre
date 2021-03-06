/* Copyright (c) 2011-2016, EPFL/Blue Brain Project
 *                          Grigori Chevtchenko <grigori.chevtchenko@epfl.ch>
 *
 * This file is part of Livre <https://github.com/BlueBrain/Livre>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3.0 as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <livre/eq/Channel.h>
#include <livre/eq/Event.h>
#include <livre/eq/FrameGrabber.h>

#include <eq/image.h>

#ifdef LIVRE_USE_LIBJPEGTURBO
#  include <turbojpeg.h>
#endif

namespace livre
{

FrameGrabber::FrameGrabber()
    : ResultImageListener()
#ifdef LIVRE_USE_LIBJPEGTURBO
    , _compressor( tjInitCompress() )
#else
    , _compressor( nullptr )
#endif
{}

FrameGrabber::~FrameGrabber()
{
#ifdef LIVRE_USE_LIBJPEGTURBO
    if( _compressor )
        tjDestroy(_compressor);
#endif
}

uint8_t* FrameGrabber::_encodeJpeg( const uint32_t width  LIVRE_UNUSED,
                                    const uint32_t height  LIVRE_UNUSED,
                                    const uint8_t* rawData  LIVRE_UNUSED,
                                    unsigned long& dataSize  LIVRE_UNUSED )
{
#ifdef LIVRE_USE_LIBJPEGTURBO
    uint8_t* tjSrcBuffer = const_cast< uint8_t* >(rawData);
    const int32_t pixelFormat = TJPF_BGRA;
    const int32_t color_components = 4; // Color Depth
    const int32_t tjPitch = width * color_components;
    const int32_t tjPixelFormat = pixelFormat;

    uint8_t* tjJpegBuf = 0;
    const int32_t tjJpegSubsamp = TJSAMP_444;
    const int32_t tjJpegQual = 100; // Image Quality
    const int32_t tjFlags = TJXOP_ROT180;

    const int32_t success =
        tjCompress2( _compressor, tjSrcBuffer, width, tjPitch, height,
                     tjPixelFormat, &tjJpegBuf, &dataSize, tjJpegSubsamp,
                     tjJpegQual, tjFlags);

    if(success != 0)
    {
        LBERROR << "libjpeg-turbo image conversion failure" << std::endl;
        return 0;
    }
    return static_cast<uint8_t *>(tjJpegBuf);
#else
    if( !_compressor ) // just to silence unused private field warning
        return nullptr;
    return nullptr;
#endif
}

void FrameGrabber::notifyNewImage( eq::Channel& channel,
                                   const eq::Image& image )
{
    const uint64_t size = image.getPixelDataSize( eq::Frame::BUFFER_COLOR );
    const uint8_t* data = image.getPixelPointer( eq::Frame::BUFFER_COLOR );
    const eq::PixelViewport& pvp = image.getPixelViewport();

    unsigned long jpegSize = size;
    uint8_t* jpegData = _encodeJpeg( pvp.w, pvp.h, data, jpegSize );

    if( !jpegData )
    {
        jpegSize = 0;
        LBERROR << "Returning an empty jpeg image" << std::endl;
    }
    channel.getConfig()->sendEvent( GRAB_IMAGE )
        << uint64_t( jpegSize ) << co::Array< const uint8_t >( jpegData,
                                                               jpegSize );
#ifdef LIVRE_USE_LIBJPEGTURBO
    tjFree(jpegData);
#endif
}

}
