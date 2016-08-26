/* Copyright (c) 2011-2015, EPFL/Blue Brain Project
 *                     Ahmet Bilgili <ahmet.bilgili@epfl.ch>
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

#include <livre/lib/pipeline/RenderPipeline.h>
#include <livre/lib/pipeline/RenderingSetGeneratorFilter.h>
#include <livre/lib/pipeline/VisibleSetGeneratorFilter.h>
#include <livre/lib/pipeline/DataUploadFilter.h>
#include <livre/lib/pipeline/RenderFilter.h>
#include <livre/lib/pipeline/HistogramFilter.h>
#include <livre/lib/configuration/VolumeRendererParameters.h>

#include <livre/core/cache/Cache.h>
#include <livre/core/pipeline/SimpleExecutor.h>
#include <livre/core/pipeline/Pipeline.h>

#include <livre/core/render/TexturePool.h>
#include <livre/core/render/FrameInfo.h>
#include <livre/core/render/ClipPlanes.h>

namespace livre
{

namespace
{
const size_t nRenderThreads = 2;
const size_t nUploadThreads = 4;
const size_t nComputeThreads = 2;
}

struct RenderPipeline::Impl
{
    Impl( DataSource& dataSource,
          Cache& dataCache,
          Cache& textureCache,
          Cache& histogramCache,
          TexturePool& texturePool,
          ConstGLContextPtr glContext )
        : _dataSource( dataSource )
        , _dataCache( dataCache )
        , _textureCache( textureCache )
        , _histogramCache( histogramCache )
        , _texturePool( texturePool )
        , _renderExecutor( nRenderThreads, glContext )
        , _computeExecutor( nComputeThreads, glContext )
        , _uploadExecutor( nUploadThreads, glContext )
    {
    }

    void createAndConnectUploaders( Pipeline& uploadPipeline,
                                    PipeFilter& visibleSetGenerator,
                                    PipeFilter& output ) const
    {
        for( size_t i = 0; i < nUploadThreads; ++i )
        {
            std::stringstream name;
            name << "DataUploader" << i;
            PipeFilter uploader =
                    uploadPipeline.add< DataUploadFilter >( name.str(),
                                                            i,
                                                            nUploadThreads,
                                                            _dataCache,
                                                            _textureCache,
                                                            _dataSource,
                                                            _texturePool );

            visibleSetGenerator.connect( "VisibleNodes",
                                         uploader, "VisibleNodes" );
            visibleSetGenerator.connect( "Params",
                                         uploader, "Params" );
            uploader.connect( "CacheObjects", output, "CacheObjects" );
        }
    }

    void createSyncPipeline( PipeFilter& renderFilter,
                             PipeFilter& histogramFilter,
                             Pipeline& renderPipeline,
                             Pipeline& uploadPipeline ) const
    {
        PipeFilter visibleSetGenerator =
                renderPipeline.add< VisibleSetGeneratorFilter >(
                    "VisibleSetGenerator", _dataSource );

        createAndConnectUploaders( uploadPipeline,
                                   visibleSetGenerator,
                                   renderFilter );

        for( size_t i = 0; i < nUploadThreads; ++i )
        {
            std::stringstream name;
            name << "DataUploader" << i;
            PipeFilter uploader =
                    static_cast< const livre::PipeFilter& >(
                        uploadPipeline.getExecutable( name.str( )));
            uploader.connect( "CacheObjects", histogramFilter, "CacheObjects" );
        }
    }

    void createAsyncPipeline( PipeFilter& renderFilter,
                              PipeFilter& redrawFilter,
                              PipeFilter& histogramFilter,
                              Pipeline& renderPipeline,
                              Pipeline& uploadPipeline ) const
    {
        PipeFilter visibleSetGenerator =
                renderPipeline.add< VisibleSetGeneratorFilter >(
                    "VisibleSetGenerator", _dataSource );

        PipeFilter renderingSetGenerator =
                renderPipeline.add< RenderingSetGeneratorFilter >(
                    "RenderingSetGenerator", _textureCache );

        visibleSetGenerator.connect( "VisibleNodes",
                                     renderingSetGenerator, "VisibleNodes" );

        renderingSetGenerator.connect( "CacheObjects",
                                      renderFilter, "CacheObjects" );

        renderingSetGenerator.connect( "CacheObjects",
                                      histogramFilter, "CacheObjects" );

        renderingSetGenerator.connect( "RenderingDone",
                                       redrawFilter, "RenderingDone" );

        createAndConnectUploaders( uploadPipeline,
                                   visibleSetGenerator,
                                   redrawFilter );
    }

    void render( const VolumeRendererParameters& vrParams,
                 const FrameInfo& frameInfo,
                 const Range& dataRange,
                 const PixelViewport& pixelViewPort,
                 const Viewport& viewport,
                 PipeFilter redrawFilter,
                 PipeFilter sendHistogramFilter,
                 const ClipPlanes& clipPlanes,
                 Renderer& renderer,
                 NodeAvailability& availability ) const
    {
        PipeFilterT< RenderFilter > renderFilter( "RenderFilter", _dataSource, renderer );
        PipeFilterT< HistogramFilter > histogramFilter( "HistogramFilter",
                                                        _histogramCache,
                                                        _dataCache,
                                                        _dataSource );
        histogramFilter.getPromise( "Frustum" ).set( frameInfo.frustum );
        histogramFilter.connect( "Histogram", sendHistogramFilter, "Histogram" );
        histogramFilter.getPromise( "RelativeViewport" ).set( viewport );
        sendHistogramFilter.getPromise( "RelativeViewport" ).set( viewport );
        sendHistogramFilter.getPromise( "Id" ).set( frameInfo.frameId );

        Pipeline renderPipeline;
        Pipeline uploadPipeline;

        if( vrParams.getSynchronousMode( ))
            createSyncPipeline( renderFilter,
                                histogramFilter,
                                renderPipeline,
                                uploadPipeline );
        else
            createAsyncPipeline( renderFilter,
                                 redrawFilter,
                                 histogramFilter,
                                 renderPipeline,
                                 uploadPipeline );

        PipeFilter visibleSetGenerator =
                static_cast< const livre::PipeFilter& >(
                    renderPipeline.getExecutable( "VisibleSetGenerator" ));

        visibleSetGenerator.getPromise( "Frustum" ).set( frameInfo.frustum );
        visibleSetGenerator.getPromise( "Frame" ).set( frameInfo.timeStep );
        visibleSetGenerator.getPromise( "DataRange" ).set( dataRange );
        visibleSetGenerator.getPromise( "Params" ).set( vrParams );
        visibleSetGenerator.getPromise( "Viewport" ).set( pixelViewPort );
        visibleSetGenerator.getPromise( "ClipPlanes" ).set( clipPlanes );

        renderFilter.getPromise( "Frustum" ).set( frameInfo.frustum );
        renderFilter.getPromise( "Viewport" ).set( pixelViewPort );
        renderFilter.getPromise( "ClipPlanes" ).set( clipPlanes );

        if( !vrParams.getSynchronousMode( ))
            redrawFilter.schedule( _renderExecutor );
        renderPipeline.schedule( _renderExecutor );
        uploadPipeline.schedule( _uploadExecutor );
        sendHistogramFilter.schedule( _computeExecutor );
        histogramFilter.schedule( _computeExecutor );
        renderFilter.execute();

        if( vrParams.getSynchronousMode( ))
        {
            const UniqueFutureMap futures( visibleSetGenerator.getPostconditions( ));
            availability.nAvailable = futures.get< NodeIds >( "VisibleNodes" ).size();
            availability.nNotAvailable = 0;
        }
        else
        {
            const PipeFilter renderingSetGenerator =
                    static_cast< const livre::PipeFilter& >(
                        renderPipeline.getExecutable( "RenderingSetGenerator" ));

            const UniqueFutureMap futures( renderingSetGenerator.getPostconditions( ));
            availability = futures.get< NodeAvailability >( "NodeAvailability" );
        }
    }

    DataSource& _dataSource;
    Cache& _dataCache;
    Cache& _textureCache;
    Cache& _histogramCache;
    TexturePool& _texturePool;
    mutable SimpleExecutor _renderExecutor;
    mutable SimpleExecutor _computeExecutor;
    mutable SimpleExecutor _uploadExecutor;
};

RenderPipeline::RenderPipeline( DataSource& dataSource,
                                Cache& dataCache,
                                Cache& textureCache,
                                Cache& histogramCache,
                                TexturePool& texturePool,
                                ConstGLContextPtr glContext )
    : _impl( new RenderPipeline::Impl( dataSource,
                                       dataCache,
                                       textureCache,
                                       histogramCache,
                                       texturePool,
                                       glContext ))
{}

RenderPipeline::~RenderPipeline()
{}

void RenderPipeline::render( const VolumeRendererParameters& vrParams,
                             const FrameInfo& frameInfo,
                             const Range& dataRange,
                             const PixelViewport& pixelViewPort,
                             const Viewport& viewport,
                             const PipeFilter& redrawFilter,
                             const PipeFilter& sendHistogramFilter,
                             const ClipPlanes& clipPlanes,
                             Renderer& renderer,
                             NodeAvailability& avaibility ) const
{
    _impl->render( vrParams,
                   frameInfo,
                   dataRange,
                   pixelViewPort,
                   viewport,
                   redrawFilter,
                   sendHistogramFilter,
                   clipPlanes,
                   renderer,
                   avaibility );
}

}