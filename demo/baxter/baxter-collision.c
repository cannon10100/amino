/* -*- mode: C; c-basic-offset: 4; -*- */
/* ex: set shiftwidth=4 tabstop=4 expandtab: */
/*
 * Copyright (c) 2015, Rice University
 * All rights reserved.
 *
 * Author(s): Neil T. Dantam <ntd@rice.edu>
 *
 *   Redistribution and use in source and binary forms, with or
 *   without modification, are permitted provided that the following
 *   conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of copyright holder the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 *   CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *   INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 *   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 *   USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 *   AND ON ANY HEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *   POSSIBILITY OF SUCH DAMAGE.
 *
 */
#define GL_GLEXT_PROTOTYPES

#include <error.h>
#include <stdio.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL.h>

#include "amino.h"
#include "amino/rx/rxtype.h"
#include "amino/rx/scenegraph.h"
#include "amino/rx/amino_gl.h"
#include "amino/rx/amino_sdl.h"
#include "amino/rx/scene_geom.h"
#include "amino/rx/scene_collision.h"

#include "baxter-demo.h"


struct display_cx {
    const struct aa_gl_globals *globals;
    const struct aa_rx_sg *scenegraph;
    struct aa_rx_cl *cl;
    double q;
    aa_rx_config_id i_q;
    struct timespec last;
};

int display( void *cx_, int updated, const struct timespec *now )
{
    struct display_cx *cx = (struct display_cx *)cx_;
    const struct aa_gl_globals *globals = cx->globals;
    const struct aa_rx_sg *scenegraph = cx->scenegraph;


    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    baxter_demo_check_error("glClearColor");

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    baxter_demo_check_error("glClear");

    aa_rx_frame_id n = aa_rx_sg_frame_count(scenegraph);
    aa_rx_frame_id m = aa_rx_sg_config_count(scenegraph);
    double q[m];
    AA_MEM_ZERO(q,m);

    if( cx->last.tv_sec || cx->last.tv_nsec ) {
        double dt = aa_tm_timespec2sec( aa_tm_sub(*now, cx->last) );
        cx->q += dt * 45 * (M_PI/180);

    }
    q[ cx->i_q ] = cx->q;

    double TF_rel[7*n];
    double TF_abs[7*n];
    aa_rx_sg_tf(scenegraph, m, q,
                n,
                TF_rel, 7,
                TF_abs, 7 );
    aa_rx_sg_render( scenegraph, globals,
                     (size_t)n, TF_abs, 7 );

    memcpy( &cx->last, now, sizeof(*now) );

    int col = aa_rx_cl_check( cx->cl, n, TF_abs, 7, NULL );
    printf("in collision: %s\n",
           col ? "yes" : "no" );

    return 1;
}

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;
    SDL_Window* window = NULL;
    SDL_GLContext gContext = NULL;
    struct aa_gl_globals *globals;

    // Initialize scene graph
    struct aa_rx_sg *scenegraph = generate_scenegraph(NULL);
    aa_rx_sg_index(scenegraph);
    aa_rx_sg_cl_init(scenegraph);

    // setup window
    baxter_demo_setup_window( scenegraph,
                              &window, &gContext, &globals );
    aa_gl_globals_set_show_visual(globals, 0);
    aa_gl_globals_set_show_collision(globals, 1);

    struct display_cx cx = {0};
    cx.globals = globals;
    cx.scenegraph = scenegraph;
    cx.i_q = aa_rx_sg_config_id(scenegraph, "left_s0");
    cx.cl = aa_rx_cl_create( scenegraph );

    {
        aa_rx_frame_id n = aa_rx_sg_frame_count(scenegraph);
        aa_rx_frame_id m = aa_rx_sg_config_count(scenegraph);
        double q[m];
        AA_MEM_ZERO(q,m);
        double TF_rel[7*n];
        double TF_abs[7*n];
        aa_rx_sg_tf(scenegraph, m, q,
                    n,
                    TF_rel, 7,
                    TF_abs, 7 );

        struct aa_rx_cl_set *allowed = aa_rx_cl_set_create( scenegraph );
        int col = aa_rx_cl_check( cx.cl, n, TF_abs, 7, allowed );
        aa_rx_cl_allow_set( cx.cl, allowed );
        aa_rx_cl_set_destroy( allowed );
    }

    aa_sdl_display_loop( window, globals,
                         display,
                         &cx );

    SDL_GL_DeleteContext(gContext);
    SDL_DestroyWindow( window );

    SDL_Quit();
    return 0;
}