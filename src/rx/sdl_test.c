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

#include "amino.h"
#include "amino/rx/amino_gl.h"
#include "amino/rx/scene_geom.h"
#include "amino/rx/scene_geom_internal.h"

const int SCREEN_WIDTH = 700;
const int SCREEN_HEIGHT = 700;

#include <SDL.h>


struct aa_rx_geom_box geom;


void Init(void)
{
    geom.base.opt.color[0] = 1;
    geom.base.opt.color[1] = 0;
    geom.base.opt.color[2] = 0;
    geom.base.opt.color[3] = 1;
    geom.base.type = AA_RX_BOX;
    geom.base.gl_buffers = NULL;
    geom.shape.dimension[0] = 0.1;
    geom.shape.dimension[1] = 0.1;
    geom.shape.dimension[2] = 0.1;

    aa_geom_gl_buffers_init( &geom.base );

}

void check_error( const char *name ){
    for (GLenum err = glGetError(); err != GL_NO_ERROR; err = glGetError()) {
        fprintf(stderr, "error %s: %d: %s\n",  name,  (int)err, gluErrorString(err));
    }
}


void display(void)
{
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    check_error("glClearColor");

    glClear(GL_COLOR_BUFFER_BIT);
    check_error("glClear");

    double world_E_model[7] = AA_TF_QUTR_IDENT_INITIALIZER;
    double world_E_camera[7] = AA_TF_QUTR_IDENT_INITIALIZER;

    world_E_model[AA_TF_QUTR_TZ] = -1;

    //world_E_camera[AA_TF_QUTR_TZ] = 1.5;
    //world_E_camera[AA_TF_QUTR_TX] = .5;
    //world_E_camera[AA_TF_QUTR_TY] = .5;

    aa_tf_yangle2quat(0 * M_PI / 180 , world_E_model );
    aa_tf_yangle2quat(15 * M_PI / 180 , world_E_camera );

    //aa_tf_yangle2quat(M_PI/4, world_E_model );

    /* { */
    /*     double eye[3] = {0,0,2}; */
    /*     double target[3] = {0,0,0}; */
    /*     double up[3] = {0,1,0}; */
    /*     aa_tf_qutr_mzlook(eye, target, up, world_E_camera ); */
    /*     } */
    //aa_gl_draw_tf( E, &buffers );
    GLfloat P[16] = {0};
    for( size_t i = 0; i < 4; i ++ ) {
        P[i*4 + i] = 1;
    }
    aa_gl_mat_perspective(M_PI_2, ((double)SCREEN_WIDTH)/SCREEN_HEIGHT,
                          0.1, 100,
                          P );
    aa_gl_draw_tf( P, world_E_camera, world_E_model, geom.base.gl_buffers );


}

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;
    SDL_Window* window = NULL;


    if( SDL_Init( SDL_INIT_VIDEO ) < 0 ) {
        printf( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
        abort();
    }


    //SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);


    window = SDL_CreateWindow( "SDL Test",
                               SDL_WINDOWPOS_UNDEFINED,
                               SDL_WINDOWPOS_UNDEFINED,
                               SCREEN_WIDTH,
                               SCREEN_HEIGHT,
                               SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL );
    if( window == NULL ) {
        printf( "Window could not be created! SDL_Error: %s\n", SDL_GetError() );
        abort();
    }

    SDL_GLContext gContext = SDL_GL_CreateContext( window );
    if( gContext == NULL )
    {
        printf( "OpenGL context could not be created! SDL Error: %s\n", SDL_GetError() );
        abort();
    }

    printf("version: %s\n", glGetString(GL_VERSION));

    Init();

    display();

    SDL_UpdateWindowSurface( window );
    SDL_GL_SwapWindow(window );
    SDL_UpdateWindowSurface( window );

    SDL_Delay( 1000 );

    SDL_GL_DeleteContext(gContext);
    SDL_DestroyWindow( window );

    SDL_Quit();
    return 0;
}
