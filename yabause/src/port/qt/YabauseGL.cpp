/*  Copyright 2005 Guillaume Duhamel
	Copyright 2005-2006 Theo Berkau
	Copyright 2008 Filipe Azevedo <pasnox@gmail.com>

	This file is part of Yabause.

	Yabause is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Yabause is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Yabause; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/
#include "YabauseGL.h"
#include "QtYabause.h"
#include <QKeyEvent>

#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
YabauseGL::YabauseGL( ) : QOpenGLWindow()
{
  QSurfaceFormat format;
#else
YabauseGL::YabauseGL( ) : QGLWidget()
{
  QGLFormat format;
#endif
  format.setDepthBufferSize(24);
  format.setStencilBufferSize(8);
  format.setRedBufferSize(8);
  format.setGreenBufferSize(8);
  format.setBlueBufferSize(8);
  format.setAlphaBufferSize(8);
#if QT_VERSION >= QT_VERSION_CHECK(5, 3, 0)
  format.setSwapInterval(0);

#ifdef _OGL3_
  format.setMajorVersion(4);
  format.setMinorVersion(3);
#endif

#ifdef _OGLES3_
  format.setMajorVersion(3);
  format.setMinorVersion(0);
  format.setRenderableType(QSurfaceFormat::OpenGLES);
#endif

#ifdef _OGLES31_
  format.setMajorVersion(3);
  format.setMinorVersion(1);
  format.setRenderableType(QSurfaceFormat::OpenGLES);
#endif
  format.setProfile(QSurfaceFormat::CoreProfile);
#else
  //format.setRenderableType(QGLFormat::OpenGL);
  format.setProfile(QGLFormat::CoreProfile);
#endif
  setFormat(format);
}

void YabauseGL::initializeGL()
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 3, 0)
  initializeOpenGLFunctions();
#endif
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glInitialized();
}

void YabauseGL::swapBuffers()
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 3, 0)
  context()->makeCurrent(context()->surface());
  context()->swapBuffers(context()->surface());
#else
  context()->swapBuffers();
#endif
}

void YabauseGL::resizeGL( int w, int h )
{ updateView( QSize( w, h ) ); }

void YabauseGL::updateView( const QSize& s )
{
	const QSize size = s.isValid() ? s : this->size();
	if ( VIDCore ) {
          VIDCore->Resize(0, 0, size.width(), size.height(), 0);
        }
}

void YabauseGL::keyPressEvent( QKeyEvent* e )
{
  PerKeyDown( e->key() );
}

void YabauseGL::keyReleaseEvent( QKeyEvent* e )
{
  PerKeyUp( e->key() );
}


extern "C"{
	int YuiRevokeOGLOnThisThread(){
		// Todo: needs to imp for async rendering
		return 0;
	}

	int YuiUseOGLOnThisThread(){
		// Todo: needs to imp for async rendering
		return 0;
	}
}
