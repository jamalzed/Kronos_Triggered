/*  Copyright 2005 Guillaume Duhamel
	Copyright 2005-2006, 2013 Theo Berkau
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

	Lightgun support fixed by JamalZ (2022).
	If you like this program, kindly consider maintaining support for older compilers
	and donating Sega Saturn and Super Nintendo software and hardware to help me,
	so I can work on similar projects to benefit the gaming community.
	Github: jamalzed		Tw: jamal_zedman	RH: Jamal
*/
#ifdef WIN32
#include <windows.h>
#endif
#include "UIYabause.h"
#include "../Settings.h"
#include "../VolatileSettings.h"
#include "UISettings.h"
#include "UIBackupRam.h"
#include "UICheats.h"
#include "UICheatSearch.h"
#include "UIDebugSH2.h"
#include "UIDebugVDP1.h"
#include "UIDebugVDP2.h"
#include "UIDebugM68K.h"
#include "UIDebugSCUDSP.h"
#include "UIDebugSCSP.h"
#include "UIDebugSCSPChan.h"
#include "UIDebugSCSPDSP.h"
#include "UIMemoryEditor.h"
#include "UIMemoryTransfer.h"
#include "UIAbout.h"
#include "../YabauseGL.h"
#include "../QtYabause.h"
#include "../CommonDialogs.h"

#include <QKeyEvent>
#include <QTextEdit>
#include <QDockWidget>
#include <QImageWriter>
#include <QUrl>
#include <QDesktopServices>
#include <QDateTime>

#include <QDebug>
#include <QMimeData>

extern "C" {
extern VideoInterface_struct *VIDCoreList[];
}

//#define USE_UNIFIED_TITLE_TOOLBAR

UIYabause::UIYabause( QWidget* parent )
	: QMainWindow( parent )
{
	mInit = false;
   search.clear();
	searchType = 0;

	// setup dialog
	setupUi( this );
	toolBar->insertAction( aFileSettings, mFileSaveState->menuAction() );
	toolBar->insertAction( aFileSettings, mFileLoadState->menuAction() );
	toolBar->insertSeparator( aFileSettings );
	setAttribute( Qt::WA_DeleteOnClose );
#ifdef USE_UNIFIED_TITLE_TOOLBAR
	setUnifiedTitleAndToolBarOnMac( true );
#endif
	fSound->setParent( 0, Qt::Popup );
	fVideoDriver->setParent( 0, Qt::Popup );
	fSound->installEventFilter( this );
	fVideoDriver->installEventFilter( this );
	// fill combo driver
	cbVideoDriver->blockSignals( true );
	for ( int i = 0; VIDCoreList[i] != NULL; i++ )
		cbVideoDriver->addItem( VIDCoreList[i]->Name, VIDCoreList[i]->id );
	cbVideoDriver->blockSignals( false );
	// create glcontext
	mYabauseGL = new YabauseGL( );
	// and set it as central application widget
#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
	QWidget *container = QWidget::createWindowContainer(mYabauseGL, this);
	container->setFocusPolicy( Qt::StrongFocus );
	setFocusPolicy( Qt::StrongFocus );
	container->setFocusProxy( this );
	container->setAcceptDrops(false);
	container->installEventFilter( this );
#else
	setFocusPolicy( Qt::StrongFocus );
	setFocusProxy( this );
	setAcceptDrops(false);
	installEventFilter( this );
#endif
	mYabauseGL->installEventFilter( this );

	this->setAcceptDrops(true);

	//bind auto start to trigger when gl is initialized. before this emulation will fail due to missing GL context
	connect(mYabauseGL, &YabauseGL::glInitialized, [&]
	{
		auto const * const vs = QtYabause::volatileSettings();
		if (vs->value("autostart").toBool())
			aEmulationRun->trigger();
	});

#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
	setCentralWidget( container );
#else
	setCentralWidget( mYabauseGL );
#endif
	oldMouseX = oldMouseY = 0;
	mouseCaptured = false;

	// create emulator thread
	mYabauseThread = new YabauseThread( this );
	// create hide mouse timer
	hideMouseTimer = new QTimer();
	// create mouse cursor timer
	mouseCursorTimer = new QTimer();
	// connections
	connect( mYabauseThread, SIGNAL( requestSize( const QSize& ) ), this, SLOT( sizeRequested( const QSize& ) ) );
	connect( mYabauseThread, SIGNAL( requestFullscreen( bool ) ), this, SLOT( fullscreenRequested( bool ) ) );
	connect( mYabauseThread, SIGNAL( requestVolumeChange( int ) ), this, SLOT( on_sVolume_valueChanged( int ) ) );
	connect( mYabauseThread, SIGNAL( error( const QString&, bool ) ), this, SLOT( errorReceived( const QString&, bool ) ) );
	connect( mYabauseThread, SIGNAL( pause( bool ) ), this, SLOT( pause( bool ) ) );
	connect( mYabauseThread, SIGNAL( reset() ), this, SLOT( reset() ) );
	connect( hideMouseTimer, SIGNAL( timeout() ), this, SLOT( hideMouse() ));
	connect( mouseCursorTimer, SIGNAL( timeout() ), this, SLOT( cursorRestore() ));
	connect( mYabauseThread, SIGNAL( toggleEmulateGun( bool ) ), this, SLOT( toggleEmulateGun( bool ) ) );
	connect( mYabauseThread, SIGNAL( toggleEmulateMouse( bool ) ), this, SLOT( toggleEmulateMouse( bool ) ) );
#if defined(MULTI_MOUSE) && defined(WIN32) && QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
	// (Untested code): connect winEvent to the window (Qt 5.0+ uses nativeEvent, Qt < 5.0 uses winEvent)
	connect( this, SIGNAL(eventData(const QString &)), this, SLOT(handleEventData(const QString &)));
#endif

	// Load shortcuts
	VolatileSettings* vs = QtYabause::volatileSettings();
	QList<QAction *> actions = findChildren<QAction *>();
	foreach ( QAction* action, actions )
	{
		if (action->text().isEmpty())
			continue;

		QString text = vs->value(QString("Shortcuts/") + action->text(), "").toString();
		if (text.isEmpty())
			continue;
		action->setShortcut(text);
	}

	// retranslate widgets
	QtYabause::retranslateWidget( this );

	QList<QAction *> actionList = menubar->actions();
	for(int i = 0;i < actionList.size();i++) {
		addAction(actionList.at(i));
	}

	restoreGeometry( vs->value("General/Geometry" ).toByteArray() );
#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
	container->setMouseTracking(true);
#else
	setMouseTracking(true);
#endif
	setMouseTracking(true);
	mouseXRatio = mouseYRatio = 1.0;
	emulateGun = false;
	emulateMouse = false;
	mouseSensitivity = vs->value( "Input/GunMouseSensitivity", 100 ).toInt();
	showMenuBarHeight = menubar->height();
	translations = QtYabause::getTranslationList();

	mIsCdIn = false;


}

UIYabause::~UIYabause()
{
#if defined(MULTI_MOUSE) && defined(WIN32)
	//free(pRawInputDeviceList);
#endif
}

void UIYabause::showEvent( QShowEvent* e )
{
	QMainWindow::showEvent( e );

	if ( !mInit )
	{
		VolatileSettings* vs = QtYabause::volatileSettings();

		if ( vs->value( "View/Menubar" ).toInt() == BD_ALWAYSHIDE )
			menubar->hide();
		if ( vs->value( "View/Toolbar" ).toInt() == BD_ALWAYSHIDE )
			toolBar->hide();
		aEmulationVSync->setChecked( vs->value( "General/EnableVSync", 1 ).toBool() );
		aViewFPS->setChecked( vs->value( "General/ShowFPS" ).toBool() );
		mInit = true;
	}
}

void UIYabause::closeEvent( QCloseEvent* e )
{
	aEmulationPause->trigger();

	if (isFullScreen())
		// Need to switch out of full screen or the geometry settings get saved
		fullscreenRequested( false );
	Settings* vs = QtYabause::settings();
	vs->setValue( "General/Geometry", saveGeometry() );
	vs->sync();

	QMainWindow::closeEvent( e );
}

void UIYabause::keyPressEvent( QKeyEvent* e )
{
	if ((emulateGun || emulateMouse) && mouseCaptured && e->key() == Qt::Key_Escape) {
		mouseCaptured = false;
		this->setCursor(Qt::ArrowCursor);
#ifdef WIN32
		ClipCursor(NULL);
#endif
	}
	else
		PerKeyDown( e->key() );
}

void UIYabause::keyReleaseEvent( QKeyEvent* e )
{ PerKeyUp( e->key() ); }

void UIYabause::leaveEvent( QEvent* e )
{
	if ((emulateMouse || emulateGun) && mouseCaptured)
	{
#ifdef WIN32
		clip_cursor();
#else
		// lock cursor to center
		int midX = geometry().x()+(width()/2); // widget global x
		int midY = geometry().y()+menubar->height()+toolBar->height()+(height()/2); // widget global y

		QPoint newPos(midX, midY);
		this->cursor().setPos(newPos);
#endif
	}
}

void UIYabause::mousePressEvent( QMouseEvent* e )
{
	if ((emulateMouse || emulateGun) && !mouseCaptured)
	{
#ifdef WIN32
		clip_cursor();
#else
		this->setCursor(Qt::BlankCursor);
#endif
		mouseCaptured = true;
	}
#if !defined(MULTI_MOUSE)
	else
	{
		PerKeyDown( (1 << 31) | e->button() );
	}
#endif
}

void UIYabause::mouseReleaseEvent( QMouseEvent* e )
{
#if !defined(MULTI_MOUSE)
	PerKeyUp( (1 << 31) | e->button() );
#endif
}

void UIYabause::hideMouse()
{
	this->setCursor(Qt::BlankCursor);
	hideMouseTimer->stop();
}

void UIYabause::cursorRestore()
{
	this->setCursor(Qt::ArrowCursor);
	mouseCursorTimer->stop();
}

void UIYabause::mouseMoveEvent( QMouseEvent* e )
{
#if defined(MULTI_MOUSE) && defined(WIN32)
#elif defined(WIN32)
	double x = (((double)e->x() /* - geometry().x() */) / mYabauseGL->width());
	double y = (((double)e->y() /* - geometry().y() */) / mYabauseGL->height());

	if (mouseCaptured)
		PerAxisMove((1 << 30), x, y);
#else
	int midX = geometry().x()+(width()/2); // widget global x
	int midY = geometry().y()+menubar->height()+toolBar->height()+(height()/2); // widget global y

	int minAdj = mouseSensitivity/100;

	int x = (e->x()-(width()/2))*mouseXRatio;
	int y = (height()/2) - e->y();

	// Seems this was causing the problem before with Lightgun support
	// y+= menubar->height()+toolBar->height();
	y *= mouseYRatio;

	// If minimum movement is less than x, wait until next pass to apply
	if (abs(x) < minAdj) x = 0;
	if (abs(y) < minAdj) y = 0;

	//printf("Mouse move (%d, %d) : y = (%d + %d + (%d / 2) + %d) * %f--> %d\n", e->x(), e->y(), menubar->height(), toolBar->height(), height(), e->y(), mouseYRatio, y);
    //fflush(stdout);

	// Need to recalculate as Y co-ordinate/displacement appears incorrect!
	if (mouseCaptured)
		PerAxisMove((1 << 30), x, y);
#endif

	VolatileSettings* vs = QtYabause::volatileSettings();

	if (!isFullScreen())
	{
		if ((emulateMouse || emulateGun) && mouseCaptured)
		{
#ifdef WIN32
			clip_cursor();
#else
			// lock cursor to center
			QPoint newPos(midX, midY);
			this->cursor().setPos(newPos);
#endif
			this->setCursor(Qt::BlankCursor);
			return;
		}
		else
		{
			ClipCursor(NULL);
			this->setCursor(Qt::ArrowCursor);
		}
	}
	else
	{
		if ((emulateMouse || emulateGun) && mouseCaptured)
		{
#ifdef WIN32
			clip_cursor();
#endif
			this->setCursor(Qt::BlankCursor);
			return;
		}
		else if (vs->value( "View/Menubar" ).toInt() == BD_SHOWONFSHOVER)
		{
			if (e->y() < showMenuBarHeight)
				menubar->show();
			else
				menubar->hide();
		}

		hideMouseTimer->start(3 * 1000);
		this->setCursor(Qt::ArrowCursor);
	}
}

void UIYabause::resizeEvent( QResizeEvent* event )
{

    //	if (event->oldSize().width() != event->size().width())
    //fixAspectRatio(event->size().width(), event->size().height());

	QMainWindow::resizeEvent( event );
}

void UIYabause::adjustHeight(int & height)
{
   // Compensate for menubar and toolbar
   VolatileSettings* vs = QtYabause::volatileSettings();
   if (vs->value("View/Menubar").toInt() != BD_ALWAYSHIDE)
      height += menubar->height();
   if (vs->value("View/Toolbar").toInt() != BD_ALWAYSHIDE)
      height += toolBar->height();
}

void UIYabause::swapBuffers()
{
	mYabauseGL->swapBuffers();
}

bool UIYabause::eventFilter( QObject* o, QEvent* e )
{
	 if (QEvent::MouseButtonPress == e->type()) {
		 QMouseEvent* mouseEvt = (QMouseEvent*)e;
		 mousePressEvent(mouseEvt);
	 }
	 else if (QEvent::MouseButtonRelease == e->type()) {
		 QMouseEvent* mouseEvt = (QMouseEvent*)e;
		 mouseReleaseEvent(mouseEvt);
	 }
	 else if (QEvent::MouseMove == e->type()) {
		 QMouseEvent* mouseEvt = (QMouseEvent*)e;
		 mouseMoveEvent(mouseEvt);
	 }
	 else if (QEvent::KeyPress == e->type()) {
		 QKeyEvent* keyEvt = (QKeyEvent*)e;
		 keyPressEvent(keyEvt);
	 }
	 else if (QEvent::KeyRelease == e->type()) {
		QKeyEvent* keyEvt = (QKeyEvent*)e;
		keyReleaseEvent(keyEvt);
	}
	 else if ( e->type() == QEvent::Hide ) {
		 setFocus();
	 }

	return QMainWindow::eventFilter( o, e );
}

void UIYabause::errorReceived( const QString& error, bool internal )
{
	if ( internal ) {
		QtYabause::appendLog( error.toLocal8Bit().constData() );
	}
	else {
		CommonDialogs::information( error );
	}
}

void UIYabause::sizeRequested( const QSize& s )
{
	int heightOffset = toolBar->height()+menubar->height();
	int width, height;
	if (s.isNull())
	{
		return;
	}
	else
	{
		width=s.width();
		height=s.height();
	}

	mouseXRatio = 320.0 / (float)width * 2.0 * (float)mouseSensitivity / 100.0;
	mouseYRatio = 240.0 / (float)height * 2.0 * (float)mouseSensitivity / 100.0;

	// Compensate for menubar and toolbar
	VolatileSettings* vs = QtYabause::volatileSettings();
	if (vs->value( "View/Menubar" ).toInt() != BD_ALWAYSHIDE)
		height += menubar->height();
	if (vs->value( "View/Toolbar" ).toInt() != BD_ALWAYSHIDE)
		height += toolBar->height();

	resize( width, height );
}

void UIYabause::fixAspectRatio( int width , int height )
{
	int aspectRatio = QtYabause::volatileSettings()->value( "Video/AspectRatio").toInt();

      if (this->isFullScreen()) {
        mYabauseGL->resize(width, height);
      }
      else{
        int heightOffset = toolBar->height()+menubar->height();
        switch(aspectRatio) {
          case 0:
            height = 3 * ((float) width / 4);
            adjustHeight(height );
            setFixedHeight(height);
            break;
          case 2:
            height = 9 * ((float) width / 16);
            adjustHeight(height );
            setFixedHeight(height);
            break;
          default:
            break;
        }
        mouseYRatio = 240.0 / (float)height * 2.0 * (float)mouseSensitivity / 100.0;
      }
}

void UIYabause::toggleFullscreen( int width, int height, bool f, int videoFormat )
{
}

QPoint preFullscreenModeWindowPosition;

void UIYabause::fullscreenRequested( bool f )
{
	if ( isFullScreen() && !f )
	{
#ifdef USE_UNIFIED_TITLE_TOOLBAR
		setUnifiedTitleAndToolBarOnMac( true );
#endif
		showNormal();
		this->move(preFullscreenModeWindowPosition);

		VolatileSettings* vs = QtYabause::volatileSettings();
		int menubarHide = vs->value( "View/Menubar" ).toInt();
		if ( menubarHide == BD_HIDEFS ||
			  menubarHide == BD_SHOWONFSHOVER)
			menubar->show();
		if ( vs->value( "View/Toolbar" ).toInt() == BD_HIDEFS )
			toolBar->show();

		setCursor(Qt::ArrowCursor);
		hideMouseTimer->stop();
	}
	else if ( !isFullScreen() && f )
	{
#ifdef USE_UNIFIED_TITLE_TOOLBAR
		setUnifiedTitleAndToolBarOnMac( false );
#endif
		VolatileSettings* vs = QtYabause::volatileSettings();

		setMaximumSize( QWIDGETSIZE_MAX, QWIDGETSIZE_MAX );
		setMinimumSize( 0,0 );
		preFullscreenModeWindowPosition = this->pos();
		this->move(0,0);

		showFullScreen();

		if ( vs->value( "View/Menubar" ).toInt() == BD_HIDEFS )
			menubar->hide();
		if ( vs->value( "View/Toolbar" ).toInt() == BD_HIDEFS )
			toolBar->hide();

		hideMouseTimer->start(3 * 1000);
	}
	if ( aViewFullscreen->isChecked() != f )
		aViewFullscreen->setChecked( f );
	aViewFullscreen->setIcon( QIcon( f ? ":/actions/no_fullscreen.png" : ":/actions/fullscreen.png" ) );
}

void UIYabause::refreshStatesActions()
{
	// reset save actions
	foreach ( QAction* a, findChildren<QAction*>( QRegExp( "aFileSaveState*" ) ) )
	{
		if ( a == aFileSaveStateAs )
			continue;
		int i = a->objectName().remove( "aFileSaveState" ).toInt();
		a->setText( QString( "%1 ... " ).arg( i ) );
		a->setToolTip( a->text() );
		a->setStatusTip( a->text() );
		a->setData( i );
	}
	// reset load actions
	foreach ( QAction* a, findChildren<QAction*>( QRegExp( "aFileLoadState*" ) ) )
	{
		if ( a == aFileLoadStateAs )
			continue;
		int i = a->objectName().remove( "aFileLoadState" ).toInt();
		a->setText( QString( "%1 ... " ).arg( i ) );
		a->setToolTip( a->text() );
		a->setStatusTip( a->text() );
		a->setData( i );
		a->setEnabled( false );
	}
	// get states files of this game
	const QString serial = QtYabause::getCurrentCdSerial();
	const QString mask = QString( "%1_*.yss" ).arg( serial );
	const QString statesPath = QtYabause::volatileSettings()->value( "General/SaveStates", getDataDirPath() ).toString();
	QRegExp rx( QString( mask ).replace( '*', "(\\d+)") );
	QDir d( statesPath );
	foreach ( const QFileInfo& fi, d.entryInfoList( QStringList( mask ), QDir::Files | QDir::Readable, QDir::Name | QDir::IgnoreCase ) )
	{
		if ( rx.exactMatch( fi.fileName() ) )
		{
			int slot = rx.capturedTexts().value( 1 ).toInt();
			const QString caption = QString( "%1 %2 " ).arg( slot ).arg( fi.lastModified().toString( Qt::SystemLocaleDate ) );
			// update save state action
			if ( QAction* a = findChild<QAction*>( QString( "aFileSaveState%1" ).arg( slot ) ) )
			{
				a->setText( caption );
				a->setToolTip( caption );
				a->setStatusTip( caption );
				// update load state action
				a = findChild<QAction*>( QString( "aFileLoadState%1" ).arg( slot ) );
				a->setText( caption );
				a->setToolTip( caption );
				a->setStatusTip( caption );
				a->setEnabled( true );
			}
		}
	}
}

void UIYabause::on_aFileSettings_triggered()
{
	Settings *s = (QtYabause::settings());
	QHash<QString, QVariant> hash;
	const QStringList keys = s->allKeys();
	Q_FOREACH(QString key, keys) {
		hash[key] = s->value(key);
	}

	YabauseLocker locker( mYabauseThread );
	if ( UISettings(&translations, window() ).exec() )
	{
		VolatileSettings* vs = QtYabause::volatileSettings();
		aEmulationVSync->setChecked( vs->value( "General/EnableVSync", 1 ).toBool() );
		aViewFPS->setChecked( vs->value( "General/ShowFPS" ).toBool() );
		mouseSensitivity = vs->value( "Input/GunMouseSensitivity" ).toInt();

		if(isFullScreen())
		{
			if ( vs->value( "View/Menubar" ).toInt() == BD_HIDEFS || vs->value( "View/Menubar" ).toInt() == BD_ALWAYSHIDE )
				menubar->hide();
			else
				menubar->show();

			if ( vs->value( "View/Toolbar" ).toInt() == BD_HIDEFS || vs->value( "View/Toolbar" ).toInt() == BD_ALWAYSHIDE )
				toolBar->hide();
			else
				toolBar->show();
		}
		else
		{
			if ( vs->value( "View/Menubar" ).toInt() == BD_ALWAYSHIDE )
				menubar->hide();
			else
				menubar->show();

			if ( vs->value( "View/Toolbar" ).toInt() == BD_ALWAYSHIDE )
				toolBar->hide();
			else
				toolBar->show();
		}


		//only reset if bios, region, cart,  back up, mpeg, sh2, m68k are changed
		Settings *ss = (QtYabause::settings());
		QHash<QString, QVariant> newhash;
		const QStringList newkeys = ss->allKeys();
		Q_FOREACH(QString key, newkeys) {
			newhash[key] = ss->value(key);
		}
		if(newhash["General/Bios"]!=hash["General/Bios"] ||
			newhash["General/EnableEmulatedBios"]!=hash["General/EnableEmulatedBios"] ||
			newhash["STV/Region"]!=hash["STV/Region"] ||
			newhash["Cartridge/Type"]!=hash["Cartridge/Type"] ||
			newhash["Memory/Path"]!=hash["Memory/Path"] ||
			newhash["MpegROM/Path" ]!=hash["MpegROM/Path" ] ||
			newhash["Advanced/SH2Interpreter" ]!=hash["Advanced/SH2Interpreter" ] ||
         newhash["Advanced/68kCore"] != hash["Advanced/68kCore"] ||
			newhash["General/CdRom"]!=hash["General/CdRom"] ||
			newhash["General/CdRomISO"]!=hash["General/CdRomISO"] ||
		        newhash["General/SystemLanguageID"]!=hash["General/SystemLanguageID"] ||
			newhash["General/ClockSync"]!=hash["General/ClockSync"] ||
			newhash["General/FixedBaseTime"]!=hash["General/FixedBaseTime"]
		)
		{
			if ( mYabauseThread->pauseEmulation( true, true ) )
				refreshStatesActions();
			return;
		}
#ifdef HAVE_LIBMINI18N
		if(newhash["General/Translation"] != hash["General/Translation"])
		{
			mini18n_close();
			retranslateUi(this);
			if ( QtYabause::setTranslationFile() == -1 )
				qWarning( "Can't set translation file" );
			QtYabause::retranslateApplication();
		}
#endif
		if(newhash["Video/VideoCore"] != hash["Video/VideoCore"])
			on_cbVideoDriver_currentIndexChanged(newhash["Video/VideoCore"].toInt());

		if(newhash["General/ShowFPS"] != hash["General/ShowFPS"])
      SetOSDToggle(newhash["General/ShowFPS"].toBool());

		if(newhash["General/EnableVSync"] != hash["General/EnableVSync"]){
			if(newhash["General/EnableVSync"].toBool()) DisableAutoFrameSkip();
			else EnableAutoFrameSkip();
		}

		if (newhash["General/EnableMultiThreading"] != hash["General/EnableMultiThreading"] ||
			 newhash["General/NumThreads"] != hash["General/NumThreads"])
		{
			if (newhash["General/EnableMultiThreading"].toBool())
			{
				int num = newhash["General/NumThreads"].toInt() < 1 ? 1 : newhash["General/NumThreads"].toInt();
				VIDSoftSetVdp1ThreadEnable(num == 1 ? 0 : 1);
				VIDSoftSetNumLayerThreads(num);
				VIDSoftSetNumPriorityThreads(num);
			}
			else
			{
				VIDSoftSetVdp1ThreadEnable(0);
				VIDSoftSetNumLayerThreads(1);
				VIDSoftSetNumPriorityThreads(1);
			}
		}


		if (newhash["Sound/SoundCore"] != hash["Sound/SoundCore"])
			ScspChangeSoundCore(newhash["Sound/SoundCore"].toInt());

		if (newhash["Video/WindowWidth"] != hash["Video/WindowWidth"] || newhash["Video/WindowHeight"] != hash["Video/WindowHeight"] ||
          newhash["View/Menubar"] != hash["View/Menubar"] || newhash["View/Toolbar"] != hash["View/Toolbar"] ||
			 newhash["Input/GunMouseSensitivity"] != hash["Input/GunMouseSensitivity"])
			sizeRequested(QSize(newhash["Video/WindowWidth"].toInt(),newhash["Video/WindowHeight"].toInt()));

		if (newhash["Video/FullscreenWidth"] != hash["Video/FullscreenWidth"] ||
			newhash["Video/FullscreenHeight"] != hash["Video/FullscreenHeight"] ||
			newhash["Video/Fullscreen"] != hash["Video/Fullscreen"])
		{
			bool f = isFullScreen();
			if (f)
				fullscreenRequested( false );
			fullscreenRequested( f );
		}

		mYabauseThread->reloadControllers();
		refreshStatesActions();
	}
}

void UIYabause::on_aFileOpenISO_triggered()
{
        YabauseLocker locker(mYabauseThread);

	if (mIsCdIn){
		mYabauseThread->OpenTray();
		mIsCdIn = false;
	}
	else{
		const QString fn = CommonDialogs::getOpenFileName( QtYabause::volatileSettings()->value( "Recents/ISOs" ).toString(), QtYabause::translate( "Select your iso/cue/bin/zip/chd file" ), QtYabause::translate( "CD Images (*.iso *.ISO *.cue *.CUE *.bin *.BIN *.mds *.MDS *.ccd *.CCD *.zip *.ZIP *.chd *.CHD)" ) );
		loadGameFromFile(fn);
                  }
	        }

void UIYabause::on_aFileOpenCDRom_triggered()
{
	YabauseLocker locker( mYabauseThread );
	QStringList list = getCdDriveList();
	int current = list.indexOf(QtYabause::volatileSettings()->value( "Recents/CDs").toString());
	QString fn = QInputDialog::getItem(this, QtYabause::translate("Open CD Rom"),
													QtYabause::translate("Choose a cdrom drive/mount point") + ":",
													list, current, false);
	if (!fn.isEmpty())
	{
		VolatileSettings* vs = QtYabause::volatileSettings();
		const int currentCDCore = vs->value( "General/CdRom" ).toInt();
		const QString currentCdRomISO = vs->value( "General/CdRomISO" ).toString();

		QtYabause::settings()->setValue( "Recents/CDs", fn );

		vs->setValue( "autostart", false );
		vs->setValue( "General/CdRom", QtYabause::defaultCDCore().id );
		vs->setValue( "General/CdRomISO", fn );

		mYabauseThread->pauseEmulation( false, true );

		refreshStatesActions();
	}
}

void UIYabause::on_mFileSaveState_triggered( QAction* a )
{
	if ( a == aFileSaveStateAs )
		return;
	YabauseLocker locker( mYabauseThread );
	if ( YabSaveStateSlot( QtYabause::volatileSettings()->value( "General/SaveStates", getDataDirPath() ).toString().toLatin1().constData(), a->data().toInt() ) != 0 )
		CommonDialogs::information( QtYabause::translate( "Couldn't save state file" ) );
	else
		refreshStatesActions();
}

void UIYabause::on_mFileLoadState_triggered( QAction* a )
{
	if ( a == aFileLoadStateAs )
		return;
	YabauseLocker locker( mYabauseThread );
	if ( YabLoadStateSlot( QtYabause::volatileSettings()->value( "General/SaveStates", getDataDirPath() ).toString().toLatin1().constData(), a->data().toInt() ) != 0 )
		CommonDialogs::information( QtYabause::translate( "Couldn't load state file" ) );
}

void UIYabause::on_aFileSaveStateAs_triggered()
{
	YabauseLocker locker( mYabauseThread );
	const QString fn = CommonDialogs::getSaveFileName( QtYabause::volatileSettings()->value( "General/SaveStates", getDataDirPath() ).toString(), QtYabause::translate( "Choose a file to save your state" ), QtYabause::translate( "Kronos Save State (*.yss)" ) );
	if ( fn.isNull() )
		return;
	if ( YabSaveState( fn.toLatin1().constData() ) != 0 )
		CommonDialogs::information( QtYabause::translate( "Couldn't save state file" ) );
}

void UIYabause::on_aFileLoadStateAs_triggered()
{
	YabauseLocker locker( mYabauseThread );
	const QString fn = CommonDialogs::getOpenFileName( QtYabause::volatileSettings()->value( "General/SaveStates", getDataDirPath() ).toString(), QtYabause::translate( "Select a file to load your state" ), QtYabause::translate( "Kronos Save State (*.yss)" ) );
	if ( fn.isNull() )
		return;
	if ( YabLoadState( fn.toLatin1().constData() ) != 0 )
		CommonDialogs::information( QtYabause::translate( "Couldn't load state file" ) );
	else
		aEmulationRun->trigger();
}

void UIYabause::on_aFileScreenshot_triggered()
{
	YabauseLocker locker( mYabauseThread );

#if defined(HAVE_LIBGL) && !defined(QT_OPENGL_ES_1) && !defined(QT_OPENGL_ES_2)
	glReadBuffer(GL_FRONT);
#endif

	QFileInfo const fileInfo(QtYabause::volatileSettings()->value("General/CdRomISO").toString());

	// take screenshot of gl view
#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
	QImage const screenshot = mYabauseGL->grabFramebuffer();
#else
	QImage const screenshot = mYabauseGL->grabFrameBuffer();
#endif

	auto const directory = QtYabause::volatileSettings()->value(QtYabause::SettingKeys::ScreenshotsDirectory, QtYabause::DefaultPaths::Screenshots()).toString();
	auto const format = QtYabause::volatileSettings()->value(QtYabause::SettingKeys::ScreenshotsFormat, "png").toString();

	// request a file to save to to user
	QString s = directory + "/" + fileInfo.baseName() + QString("_%1." + format).arg(QDateTime::currentDateTime().toString("dd_MM_yyyy-hh_mm_ss"));

	// if the user didn't provide a filename extension, we force it to png
	QFileInfo qfi( s );
	if ( qfi.suffix().isEmpty() )
		s += ".png";

	// write image if ok
	if ( !s.isEmpty() )
	{
		QImageWriter iw( s );
		if ( !iw.write( screenshot ))
		{
			CommonDialogs::information( QtYabause::translate( "An error occur while writing the screenshot: " + iw.errorString()) );
		}
	}
}

void UIYabause::on_aFileQuit_triggered()
{ close(); }

void UIYabause::on_aEmulationRun_triggered()
{
	if ( mYabauseThread->emulationPaused() )
	{
		mYabauseThread->pauseEmulation( false, false );
		refreshStatesActions();
		if (isFullScreen())
			hideMouseTimer->start(3 * 1000);
	}
}

void UIYabause::on_aEmulationPause_triggered()
{
	if ( !mYabauseThread->emulationPaused() )
		mYabauseThread->pauseEmulation( true, false );
}

void UIYabause::on_aEmulationReset_triggered()
{ mYabauseThread->resetEmulation(); }

void UIYabause::on_aEmulationVSync_toggled( bool toggled )
{
	Settings* vs = QtYabause::settings();
	vs->setValue( "General/EnableVSync", toggled );
	vs->sync();

	if ( !toggled )
		EnableAutoFrameSkip();
	else
		DisableAutoFrameSkip();
}

void UIYabause::on_aToolsBackupManager_triggered()
{
	YabauseLocker locker( mYabauseThread );
	if ( mYabauseThread->init() < 0 )
	{
		CommonDialogs::information( QtYabause::translate( "Kronos is not initialized, can't manage backup ram." ) );
		return;
	}
	UIBackupRam( this ).exec();
}

void UIYabause::on_aToolsCheatsList_triggered()
{
	YabauseLocker locker( mYabauseThread );
	UICheats( this ).exec();
}

void UIYabause::on_aToolsCheatSearch_triggered()
{
   YabauseLocker locker( mYabauseThread );
   UICheatSearch cs(this, &search, searchType);

   cs.exec();

   search = *cs.getSearchVariables( &searchType);
}

void UIYabause::on_aToolsTransfer_triggered()
{
	YabauseLocker locker( mYabauseThread );
	UIMemoryTransfer( mYabauseThread, this ).exec();
}

void UIYabause::on_aViewFPS_triggered( bool toggled )
{
	Settings* vs = QtYabause::settings();
	vs->setValue( "General/ShowFPS", toggled );
	vs->sync();
	SetOSDToggle(toggled ? 1 : 0);
}

void UIYabause::on_aViewLayerVdp1_triggered()
{ ToggleVDP1(); }

void UIYabause::on_aViewLayerNBG0_triggered()
{ ToggleNBG0(); }

void UIYabause::on_aViewLayerNBG1_triggered()
{ ToggleNBG1(); }

void UIYabause::on_aViewLayerNBG2_triggered()
{ ToggleNBG2(); }

void UIYabause::on_aViewLayerNBG3_triggered()
{ ToggleNBG3(); }

void UIYabause::on_aViewLayerRBG0_triggered()
{ ToggleRBG0(); }

void UIYabause::on_aViewLayerRBG1_triggered()
{ ToggleRBG1(); }

void UIYabause::on_aViewFullscreen_triggered( bool b )
{
	fullscreenRequested( b );
}

void UIYabause::breakpointHandlerMSH2(bool displayMessage)
{
	YabauseLocker locker( mYabauseThread );
	if (displayMessage)
		CommonDialogs::information( QtYabause::translate( "Breakpoint Reached" ) );
	UIDebugSH2(UIDebugCPU::PROC_MSH2, mYabauseThread, this ).exec();
}

void UIYabause::breakpointHandlerSSH2(bool displayMessage)
{
	YabauseLocker locker( mYabauseThread );
	if (displayMessage)
		CommonDialogs::information( QtYabause::translate( "Breakpoint Reached" ) );
	UIDebugSH2(UIDebugCPU::PROC_SSH2, mYabauseThread, this ).exec();
}

void UIYabause::breakpointHandlerM68K()
{
	YabauseLocker locker( mYabauseThread );
	CommonDialogs::information( QtYabause::translate( "Breakpoint Reached" ) );
	UIDebugM68K( mYabauseThread, this ).exec();
}

void UIYabause::breakpointHandlerSCUDSP()
{
	YabauseLocker locker( mYabauseThread );
	CommonDialogs::information( QtYabause::translate( "Breakpoint Reached" ) );
	UIDebugSCUDSP( mYabauseThread, this ).exec();
}

void UIYabause::breakpointHandlerSCSPDSP()
{
	YabauseLocker locker( mYabauseThread );
	CommonDialogs::information( QtYabause::translate( "Breakpoint Reached" ) );
	UIDebugSCSPDSP( mYabauseThread, this ).exec();
}

void UIYabause::on_aViewDebugMSH2_triggered()
{
	YabauseLocker locker( mYabauseThread );
	UIDebugSH2( UIDebugCPU::PROC_MSH2, mYabauseThread, this ).exec();
}

void UIYabause::on_aViewDebugSSH2_triggered()
{
	YabauseLocker locker( mYabauseThread );
	UIDebugSH2( UIDebugCPU::PROC_SSH2, mYabauseThread, this ).exec();
}

void UIYabause::on_aViewDebugVDP1_triggered()
{
	YabauseLocker locker( mYabauseThread );
	UIDebugVDP1( this ).exec();
}

void UIYabause::on_aViewDebugVDP2_triggered()
{
	YabauseLocker locker( mYabauseThread );
	UIDebugVDP2( this ).exec();
}

void UIYabause::on_aHelpReport_triggered()
{
	QDesktopServices::openUrl(QUrl(aHelpReport->statusTip()));
}

void UIYabause::on_aViewDebugM68K_triggered()
{
	YabauseLocker locker( mYabauseThread );
	UIDebugM68K( mYabauseThread, this ).exec();
}

void UIYabause::on_aViewDebugSCUDSP_triggered()
{
	YabauseLocker locker( mYabauseThread );
	UIDebugSCUDSP( mYabauseThread, this ).exec();
}

void UIYabause::on_aViewDebugSCSP_triggered()
{
	YabauseLocker locker( mYabauseThread );
	UIDebugSCSP( this ).exec();
}

void UIYabause::on_aViewDebugSCSPChan_triggered()
{
      UIDebugSCSPChan(this).exec();
}

void UIYabause::on_aViewDebugSCSPDSP_triggered()
{
	YabauseLocker locker( mYabauseThread );
	UIDebugSCSPDSP( mYabauseThread, this ).exec();
}

void UIYabause::on_aViewDebugMemoryEditor_triggered()
{
	YabauseLocker locker( mYabauseThread );
	UIMemoryEditor( UIDebugCPU::PROC_MSH2, mYabauseThread, this ).exec();
}

void UIYabause::on_aTraceLogging_triggered( bool toggled )
{
#ifdef SH2_TRACE
	SH2SetInsTracing(toggled? 1 : 0);
#endif
	return;
}

void UIYabause::on_aHelpCompatibilityList_triggered()
{ QDesktopServices::openUrl( QUrl( aHelpCompatibilityList->statusTip() ) ); }

void UIYabause::on_aHelpAbout_triggered()
{
	YabauseLocker locker( mYabauseThread );
	UIAbout( window() ).exec();
}

void UIYabause::on_aSound_triggered()
{
	// show volume widget
	sVolume->setValue(QtYabause::volatileSettings()->value( "Sound/Volume").toInt());
	QWidget* ab = toolBar->widgetForAction( aSound );
	fSound->move( ab->mapToGlobal( ab->rect().bottomLeft() ) );
	fSound->show();
}

void UIYabause::on_aVideoDriver_triggered()
{
	// set current core the selected one in the combo list
	if ( VIDCore )
	{
		cbVideoDriver->blockSignals( true );
		for ( int i = 0; VIDCoreList[i] != NULL; i++ )
		{
			if ( VIDCoreList[i]->id == VIDCore->id )
			{
				cbVideoDriver->setCurrentIndex( cbVideoDriver->findData( VIDCore->id ) );
				break;
			}
		}
		cbVideoDriver->blockSignals( false );
	}
	//  show video core widget
	QWidget* ab = toolBar->widgetForAction( aVideoDriver );
	fVideoDriver->move( ab->mapToGlobal( ab->rect().bottomLeft() ) );
	fVideoDriver->show();
}

void UIYabause::on_cbSound_toggled( bool toggled )
{
	if ( toggled )
		ScspUnMuteAudio(SCSP_MUTE_USER);
	else
		ScspMuteAudio(SCSP_MUTE_USER);
	cbSound->setIcon( QIcon( toggled ? ":/actions/sound.png" : ":/actions/mute.png" ) );
}

void UIYabause::on_sVolume_valueChanged( int value )
{
	ScspSetVolume( value );
	Settings* vs = QtYabause::settings();
	vs->setValue("Sound/Volume", value );
}

void UIYabause::on_cbVideoDriver_currentIndexChanged( int id )
{
	VideoInterface_struct* core = QtYabause::getVDICore( cbVideoDriver->itemData( id ).toInt() );
	if ( core )
	{
		if ( VideoChangeCore( core->id ) == 0 )
			mYabauseGL->updateView();
	}
}

void UIYabause::pause( bool paused )
{
	mYabauseGL->updateView();

	aEmulationRun->setEnabled( paused );
	aEmulationPause->setEnabled( !paused );
	aEmulationReset->setEnabled( !paused );
}

void UIYabause::reset()
{
	mYabauseGL->updateView();
}

void UIYabause::toggleEmulateGun( bool enable )
{
	emulateGun = enable;
}

void UIYabause::toggleEmulateMouse( bool enable )
{
	emulateMouse = enable;
}

int UIYabause::loadGameFromFile(QString const& fileName)
{
	YabauseLocker locker(mYabauseThread);

	VolatileSettings* vs = QtYabause::volatileSettings();
	const int currentCDCore = vs->value("General/CdRom").toInt();
	const QString currentCdRomISO = vs->value("General/CdRomISO").toString();

	QtYabause::settings()->setValue("Recents/ISOs", fileName);

	vs->setValue("autostart", false);
	vs->setValue("General/CdRom", ISOCD.id);
	vs->setValue("General/CdRomISO", fileName);

	if (mYabauseThread->CloseTray() != 0) {
		mYabauseThread->pauseEmulation(false, true);
	}
	mIsCdIn = true;

	refreshStatesActions();
	return 0;
}

void UIYabause::dragEnterEvent(QDragEnterEvent* e)
{
	if (e->mimeData()->hasUrls()) {
		e->acceptProposedAction();
	}
}

void UIYabause::dropEvent(QDropEvent* e)
{
	auto urls = e->mimeData()->urls();
	const QUrl& url = urls.first();
	QString const& fileName = url.toLocalFile();
	qDebug() << "Dropped file:" << fileName;

	if (QFile::exists(fileName))
	{
		loadGameFromFile(fileName);
	}
}


#ifdef WIN32
void UIYabause::clip_cursor() {
	int x = mYabauseGL->x() + geometry().x(),
		y = mYabauseGL->y() + geometry().y() /*+ menubar->height() + toolBar->height()*/,
		w = mYabauseGL->width(),
		h = mYabauseGL->height();

	/*printf("yabGL (%d, %d, %d, %d) vs. geo (%d, %d, %d, %d)\n\t vs. window (%d, %d, %d, %d) vs window geo (%d, %d, %d, %d)\n\t vs. frame geo (%d, %d, %d, %d)",
		mYabauseGL->x(), mYabauseGL->y(), mYabauseGL->width(), mYabauseGL->height(),
		mYabauseGL->frameGeometry().x(), mYabauseGL->frameGeometry().y(), mYabauseGL->frameGeometry().width(), mYabauseGL->frameGeometry().height(),
		this->x(), this->y(), this->width(), this->height(),
		this->geometry().x(), this->geometry().y(), this->geometry().width(), this->geometry().height(),
		this->frameGeometry().x(), this->frameGeometry().y(), this->frameGeometry().width(), this->frameGeometry().height()
		);*/

	RECT clip = {x, y, x + w, y+ h};
		
	ClipCursor(&clip);
}

// https://stackoverflow.com/questions/37071142/get-raw-mouse-movement-in-qt
// https://stackoverflow.com/questions/1755196/receive-wm-copydata-messages-in-a-qt-app
// https://docs.microsoft.com/en-us/windows/win32/dxtecharts/taking-advantage-of-high-dpi-mouse-movement?redirectedfrom=MSDN#directinput
#ifdef MULTI_MOUSE
void UIYabause::initRawInput() {
	RAWINPUTDEVICE device;
	// Register the Raw input mouse handler
	device.usUsagePage = 0x01;
	device.usUsage = 0x02;
	device.dwFlags = 0;
	device.hwndTarget = (HWND) winId();
	if (RegisterRawInputDevices(&device, 1, sizeof device) == FALSE) {
		//registration failed. 
		printf("RawInput init failed:\n");
		fflush(stdout);
	} else {
		printf("RawInput init success!\n");
		fflush(stdout);
	}
/*	UINT nDevices;
	PRAWINPUTDEVICELIST pRawInputDeviceList;
	if (GetRawInputDeviceList(NULL, &nDevices, sizeof(RAWINPUTDEVICELIST)) != 0) {

		pRawInputDeviceList = (RAWINPUTDEVICELIST *)malloc(sizeof(RAWINPUTDEVICELIST) * nDevices);
		GetRawInputDeviceList(pRawInputDeviceList, &nDevices, sizeof(RAWINPUTDEVICELIST));
	}*/
}

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
bool UIYabause::nativeEvent(const QByteArray &eventType, void *msg, long *result) {
	MSG *message = static_cast<MSG*>(msg);
#else
bool UIYabause::winEvent(MSG *message, long *result) {
#endif
	LPBYTE lpb;// = new BYTE[dwSize];//LPBYTE lpb = new BYTE[dwSize];
	UINT dwSize;
	RAWINPUT *raw;

	if( (emulateMouse || emulateGun) && mouseCaptured) {
		fflush(stdout);
		if (message->message == WM_INPUT ) {
			printf(" - WM_INPUT");
			fflush(stdout);
			GetRawInputData((HRAWINPUT)message->lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
			lpb = (LPBYTE) malloc(sizeof(LPBYTE) * dwSize);
			if (lpb == NULL) {
				return false;
			} 
		
			if (GetRawInputData((HRAWINPUT)message->lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize ) {
				OutputDebugString (TEXT("GetRawInputData doesn't return correct size !\n"));
			}

			raw = (RAWINPUT*)lpb;

			if (raw->header.dwType == RIM_TYPEMOUSE) {
				u32 mouse_id = (1 << 31) | (((DWORD)raw->header.hDevice) << 5);

				// Handle the mouse buttons
				if (raw->data.mouse.ulRawButtons & RI_MOUSE_BUTTON_1_DOWN)
					PerKeyDown(mouse_id | 1 );
				if (raw->data.mouse.ulRawButtons & RI_MOUSE_BUTTON_1_UP)
					PerKeyUp( mouse_id | 1 );
				if (raw->data.mouse.ulRawButtons & RI_MOUSE_BUTTON_2_DOWN)
					PerKeyDown( mouse_id | 2 );
				if (raw->data.mouse.ulRawButtons & RI_MOUSE_BUTTON_2_UP)
					PerKeyUp( mouse_id | 2 );
				if (raw->data.mouse.ulRawButtons & RI_MOUSE_BUTTON_3_DOWN)
					PerKeyDown( mouse_id | 3 );
				if (raw->data.mouse.ulRawButtons & RI_MOUSE_BUTTON_3_UP)
					PerKeyUp( mouse_id | 3 );
				if (raw->data.mouse.ulRawButtons & RI_MOUSE_BUTTON_4_DOWN)
					PerKeyDown( mouse_id | 4 );
				if (raw->data.mouse.ulRawButtons & RI_MOUSE_BUTTON_4_UP)
					PerKeyUp( mouse_id | 4 );
				if (raw->data.mouse.ulRawButtons & RI_MOUSE_BUTTON_5_DOWN)
					PerKeyDown( mouse_id | 5 );
				if (raw->data.mouse.ulRawButtons & RI_MOUSE_BUTTON_5_UP)
					PerKeyUp( mouse_id | 5 );

				if (raw->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) {
					double x = (((double)raw->data.mouse.lLastX /* - geometry().x() */) / mYabauseGL->width());
					double y = (((double)raw->data.mouse.lLastY /* - geometry().y() */) / mYabauseGL->height());
					
					printf("Absolute move %f, %f", x, y);
					PerAxisMove(mouse_id | 6, x, y);
				} else {
					printf("Relative move not supported!");
					fflush(stdout);
					//PerAxisMove(mouse_id, x, y);
				}

				/*printf("Mouse:hDevice %d \n\t usFlags=%04x \n\tulButtons=%04x \n\tusButtonFlags=%04x \n\tusButtonData=%04x \n\tulRawButtons=%04x \n\tlLastX=%ld \n\tlLastY=%ld \n\tulExtraInformation=%04x\r, %ld\n",					
					raw->header.hDevice,
					raw->data.mouse.usFlags, 
					raw->data.mouse.ulButtons, 
					raw->data.mouse.usButtonFlags, 
					raw->data.mouse.usButtonData, 
					raw->data.mouse.ulRawButtons, 
					raw->data.mouse.lLastX, 
					raw->data.mouse.lLastY, 
					raw->data.mouse.ulExtraInformation);*/
				fflush(stdout);
				return true;
			} 
			free(lpb); 
		}
		printf("\n");
	}
	fflush(stdout);
	return false;
}
#endif
#endif