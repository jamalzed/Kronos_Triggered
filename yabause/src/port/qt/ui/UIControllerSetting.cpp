/*	Copyright 2008 Filipe Azevedo <pasnox@gmail.com>
   Copyright 2013 Theo Berkau <cwx@cyberwarriorx.com>

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
#include "UIPadSetting.h"
#include "UIPortManager.h"
#include "../Settings.h"

#include <QKeyEvent>
#include <QTimer>
#include <QStylePainter>
#include <QStyleOptionToolButton>

// Make a parent class for all controller setting classes


UIControllerSetting::UIControllerSetting( PerInterface_struct* core, uint port, uint pad, uint perType, QWidget* parent )
	: QDialog( parent )
{
#if defined(MULTI_MOUSE) && defined(WIN32)
	RAWINPUTDEVICE device;
#endif
	Q_ASSERT( core );
	
	mCore = core;
	mPort = port;
	mPad = pad;
	mPerType = perType;
	mTimer = new QTimer( this );
	mTimer->setInterval( 25 );
	curTb = NULL;
	mPadKey = 0;
	mlInfos = NULL;
	scanFlags = PERSF_ALL;
	QtYabause::retranslateWidget( this );
#if defined(MULTI_MOUSE) && defined(WIN32)
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
	// (Untested code): connect winEvent to the window (Qt 5.0+ uses nativeEvent, Qt < 5.0 uses winEvent)
	connect( this, SIGNAL(eventData(const QString &)), this, SLOT(handleEventData(const QString &)));
#endif
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
#endif
}

UIControllerSetting::~UIControllerSetting()
{
}

void UIControllerSetting::setInfos(QLabel *lInfos)
{
   mlInfos = lInfos;
}

void UIControllerSetting::setScanFlags(u32 scanMask)
{
	switch (mPerType)
	{
		case PERPAD:
			scanFlags = PERSF_KEY | PERSF_BUTTON | PERSF_HAT;
			break;
		case PERWHEEL:
		case PERMISSIONSTICK:
		case PER3DPAD:
		case PERTWINSTICKS:
			scanFlags = PERSF_KEY | PERSF_BUTTON | PERSF_HAT | PERSF_AXIS;
			break;
		case PERGUN:
			scanFlags = PERSF_KEY | PERSF_BUTTON | PERSF_MOUSEMOVE;
			break;
		case PERKEYBOARD:
			scanFlags = PERSF_KEY;
			break;
		case PERMOUSE:
			scanFlags = PERSF_KEY | PERSF_BUTTON | PERSF_HAT | PERSF_MOUSEMOVE;
			break;
		default:
			scanFlags = PERSF_ALL;
			break;
	}

	scanFlags &= scanMask;
	setMouseTracking(scanFlags & PERSF_MOUSEMOVE ? true : false);
}

void UIControllerSetting::keyPressEvent( QKeyEvent* e )
{
	if ( mTimer->isActive() )
	{
		if ( e->key() != Qt::Key_Escape )
		{
			setPadKey( e->key() );
		}
		else
		{
			e->ignore();
			mButtons.key( mPadKey )->setChecked( false );
			mlInfos->clear();
			mTimer->stop();
			curTb->setAttribute(Qt::WA_TransparentForMouseEvents, false);
		}
	}
	else if ( e->key() == Qt::Key_Escape )
	{
		reject();
	}
	else
	{
		QWidget::keyPressEvent( e );
	}
}

void UIControllerSetting::mouseMoveEvent( QMouseEvent * e )
{
#if defined(MULTI_MOUSE) && defined(WIN32)
	if ( ! mTimer->isActive() )
#else
	if ( mTimer->isActive() )
	{
		if (scanFlags & PERSF_MOUSEMOVE)
			setPadKey((1 << 30));
	}
	else
#endif
		QWidget::mouseMoveEvent( e );
}

void UIControllerSetting::mousePressEvent( QMouseEvent * e )
{
#if defined(MULTI_MOUSE) && defined(WIN32)
	if ( ! mTimer->isActive() )
#else
	if ( mTimer->isActive() )
	{
		if (scanFlags & PERSF_BUTTON)
			setPadKey( (1 << 31) | e->button() );
	}
	else
#endif
		QWidget::mousePressEvent( e );
}

void UIControllerSetting::setPadKey( u32 key )
{
	Q_ASSERT( mlInfos );

	const QString settingsKey = QString( UIPortManager::mSettingsKey )
		.arg( mPort )
		.arg( mPad )
		.arg( mPerType )
		.arg( mPadKey );
	
	QtYabause::settings()->setValue( settingsKey, (quint32)key );
	mButtons.key( mPadKey )->setIcon( QIcon( ":/actions/icons/actions/button_ok.png" ) );
	mButtons.key( mPadKey )->setChecked( false );
	mlInfos->clear();
	mTimer->stop();
	if (curTb)
	   curTb->setAttribute(Qt::WA_TransparentForMouseEvents, false);
	printf("key set - %x", key);
	fflush(stdout);
}

void UIControllerSetting::loadPadSettings()
{
	Settings* settings = QtYabause::settings();
	
	foreach ( const u8& name, mNames.keys() )
	{
		mPadKey = name;
		const QString settingsKey = QString( UIPortManager::mSettingsKey )
			.arg( mPort )
			.arg( mPad )
			.arg( mPerType )
			.arg( mPadKey );
		
		if ( settings->contains( settingsKey ) )
		{
			setPadKey( settings->value( settingsKey ).toUInt() );
		}
	}
}

bool UIControllerSetting::eventFilter( QObject* object, QEvent* event )
{
	if ( event->type() == QEvent::Paint )
	{
		QToolButton* tb = qobject_cast<QToolButton*>( object );
		
		if ( tb )
		{
			if ( tb->isChecked() )
			{
				QStylePainter sp( tb );
				QStyleOptionToolButton options;
				
				options.initFrom( tb );
				options.arrowType = Qt::NoArrow;
				options.features = QStyleOptionToolButton::None;
				options.icon = tb->icon();
				options.iconSize = tb->iconSize();
				options.state = QStyle::State_Enabled | QStyle::State_HasFocus | QStyle::State_On | QStyle::State_AutoRaise;
				
				sp.drawComplexControl( QStyle::CC_ToolButton, options );
				
				return true;
			}
		}
	}
	
	return false;
}

void UIControllerSetting::tbButton_clicked()
{
	QToolButton* tb = qobject_cast<QToolButton*>( sender() );
	
	if ( !mTimer->isActive() )
	{
		tb->setChecked( true );
		mPadKey = mButtons[ tb ];
	
		QString text1 = QtYabause::translate(QString("Awaiting input for"));
		QString text2 = QtYabause::translate(mNames[ mPadKey ]);
		QString text3 = QtYabause::translate(QString("Press Esc key to cancel"));

		mlInfos->setText( text1 + QString(": %1\n").arg(text2) + text3 );
		setScanFlags(mScanMasks[mPadKey]);
		mCore->Flush();
		curTb=tb;
		tb->setAttribute(Qt::WA_TransparentForMouseEvents);
		mTimer->start();
	}
	else
	{
		tb->setChecked( tb == mButtons.key( mPadKey ) );
	}
}

void UIControllerSetting::timer_timeout()
{
	u32 key = 0;
	key = mCore->Scan(scanFlags);
	
	if ( key != 0 )
	{
		setPadKey( key );
	}
}

// https://stackoverflow.com/questions/37071142/get-raw-mouse-movement-in-qt
// https://stackoverflow.com/questions/1755196/receive-wm-copydata-messages-in-a-qt-app
// https://docs.microsoft.com/en-us/windows/win32/dxtecharts/taking-advantage-of-high-dpi-mouse-movement?redirectedfrom=MSDN#directinput
#if defined(MULTI_MOUSE) && defined(WIN32)
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
bool UIControllerSetting::nativeEvent(const QByteArray &eventType, void *msg, long *result) {
	MSG *message = static_cast<MSG*>(msg);
#else
bool UIControllerSetting::winEvent(MSG *message, long *result) {
#endif
	LPBYTE lpb;// = new BYTE[dwSize];//LPBYTE lpb = new BYTE[dwSize];
	UINT dwSize;
	RAWINPUT *raw;

	if( mTimer->isActive() ) {
		if (message->message == WM_INPUT ) {
			printf("set_key");
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
					setPadKey(mouse_id | 1 );
				else if (raw->data.mouse.ulRawButtons & RI_MOUSE_BUTTON_2_DOWN)
					setPadKey( mouse_id | 2 );
				else if (raw->data.mouse.ulRawButtons & RI_MOUSE_BUTTON_3_DOWN)
					setPadKey( mouse_id | 3 );
				else if (raw->data.mouse.ulRawButtons & RI_MOUSE_BUTTON_4_DOWN)
					setPadKey( mouse_id | 4 );
				else if (raw->data.mouse.ulRawButtons & RI_MOUSE_BUTTON_5_DOWN)
					setPadKey( mouse_id | 5 );
				else if (scanFlags & PERSF_MOUSEMOVE)
					setPadKey( mouse_id | 6 );

				return true;
			} 
			free(lpb); 
			printf("\n");
			fflush(stdout);
		}
	}
	return false;
}
#endif
