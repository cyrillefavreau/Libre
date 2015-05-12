/* Copyright (c) 2011-2015, EPFL/Blue Brain Project
 *                          Ahmet Bilgili <ahmet.bilgili@epfl.ch>
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

#ifndef _View_h_
#define _View_h_

#include <livre/core/dashTypes.h>
#include <livre/core/mathTypes.h>
#include <livre/core/lunchboxTypes.h>
#include <livre/core/Render/Viewport.h>
#include <livre/core/Render/Frustum.h>
#include <boost/progress.hpp>

namespace livre
{

/**
 * The FrameInfo struct keeps the frame information.
 */
struct FrameInfo
{
    FrameInfo( const Frustum& cFrustum, const Frustum& pFrustum );
    RenderBricks renderBrickList; //!<The textures are guaranteed to be in memory.
    DashNodeVector allNodeList; //!< The list of nodes to be rendered.
    DashNodeVector renderNodeList; //!< The list of nodes to be rendered.
    DashNodeVector notAvailableRenderNodeList; //<! The unavailable nodes for rendering.
    const Frustum& previousFrustum; //!< The previous frustum. Some algorithms can use the previous frustum.
    const Frustum& currentFrustum; //!< The current frustum.
};

/**
 * The View class is a viewport on the rendering widgets frame buffer.
 */
class View
{

public:

    View( );

    virtual ~View( );

    /**
     * @param rendererPtr Sets the renderer.
     */
    void setRenderer( RendererPtr rendererPtr );

    /**
     * @param viewport Sets the viewport in the ( 0.f, 0.f, 1.f, 1.f ) normalized coordinates.
     */
    void setViewport( const Viewportf &viewport );

    /**
     * @return The renderer.
     */
    RendererPtr getRenderer() const;

    /**
     * @return The normalized viewport.
     */
    const Viewportf& getViewport() const;

    /**
     * Renders the viewport on widget, using the renderer, using the list generated by
     * render list generator.
     * @param widget The framebuffer widget to render to.
     * @param renderListGenerator The renderlist generator.
     */
    virtual void render( const GLWidget& widget,
                         GenerateRenderingSet& renderListGenerator );

    /**
     * @param frustum Derived class should implement the get frustum method.
     */
    virtual const Frustum& getFrustum() const = 0;

protected:

    /**
     * Is called after the render list generated, and before the rendering.
     * @param widget The widget to render to.
     * @param frameInfo Frame information.
     * @param renderListGenerator The render list is generated by renderListGenerator.
     * @param modifiedFrustum The frustum can be modified before rendering.
     * @return If returned false, the frame is not going to be rendered.
     */
    virtual bool onPreRender_( const GLWidget& widget,
                               const FrameInfo& frameInfo,
                               GenerateRenderingSet& renderListGenerator,
                               Frustum& modifiedFrustum );

    /**
     * Is called after the rendering.
     * @param rendered Result of the prerender method.
     * @param widget The widget to render to.
     * @param frameInfo Frame information.
     * @param renderListGenerator The render list is generated by renderListGenerator.
     */
    virtual void onPostRender_( const bool rendered,
                                const GLWidget& widget,
                                const FrameInfo& frameInfo,
                                GenerateRenderingSet& renderListGenerator );

    RendererPtr rendererPtr_; //!< Renderer.
    Viewportf viewport_; //!< The normalized viewport.

private:

    Frustum previousFrustum_; //!< Previous rendering frustum. Some algorithms can do calculations according to movement
    Frustum currentFrustum_; //!< The current rendering frustum.
    boost::progress_display progress_;
};

}

#endif // _View_h_