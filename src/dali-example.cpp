/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

// EXTERNAL INCLUDES
#include <dali-toolkit/dali-toolkit.h>
#include <dali-toolkit/devel-api/controls/video-view/video-view-devel.h>
#include <dali/devel-api/adaptor-framework/window-devel.h>
#include <dali/public-api/adaptor-framework/window.h>
#include <dali/integration-api/debug.h>

using namespace Dali;
using namespace Dali::Toolkit;

namespace
{
const char * const STYLE_PATH( DEMO_STYLE_DIR "dali-example.json" ); ///< The style used for this example
const char * const BACKGROUND_STYLE_NAME( "Background" ); ///< The name of the Background style
const char * const IMAGE_STYLE_NAME( "StyledImage" ); ///< The name of the styled image style
const char * const IMAGE_PATH( DEMO_IMAGE_DIR "silhouette.jpg" ); ///< Image to show

const int INIT_WIDTH( 500 );
const int INIT_HEIGHT( 500 );

const char* const PLAY_FILE = "/usr/apps/com.samsung.dali-example/res/earth.mp4";
const char* const PLAY_FILE1 = "/usr/apps/com.samsung.dali-example/res/bubble.avi";
const char* const PLAY_FILE2 = "/usr/apps/com.samsung.dali-example/res/demoVideo.mp4";
const char* const PLAY_FILE3 = "/usr/apps/com.samsung.dali-example/res/test.mp4";
} // unnamed namespace

/// Basic DALi Example to use for debugging small programs on target
class Example : public ConnectionTracker
{
public:

  ///< Constructor
  Example( Application& application )
  : mApplication( application ),
    mCurrentPosition(0,0)
  {
    mApplication.InitSignal().Connect( this, &Example::Create );
  }

  ~Example() = default;

private:

  ///< Called to initialise the application UI
  void Create( Application& application )
  {
    // Get a handle to the main window & respond to key events
    Window window = application.GetWindow();
    window.KeyEventSignal().Connect( this, &Example::OnKeyEvent );
   // window.TouchedSignal().Connect(this, &Example::OnTouched);
    window.GetRootLayer().TouchedSignal().Connect(this, &Example::OnTouched);

    DevelWindow::SetPositionSize( window, PositionSize( 0, 0, INIT_WIDTH, INIT_HEIGHT ));
    window.SetBackgroundColor(Color::RED);

#if 1
    //mVideoView1 = DevelVideoView::New( Dali::VideoSyncMode::ENABLED );
    mVideoView1 = VideoView::New();
    window.Add( mVideoView1 );
    mVideoView1.SetProperty( Actor::Property::PARENT_ORIGIN, ParentOrigin::TOP_CENTER );
    mVideoView1.SetProperty( Actor::Property::ANCHOR_POINT, AnchorPoint::TOP_CENTER );
    mVideoView1.SetResizePolicy( ResizePolicy::FIXED, Dimension::ALL_DIMENSIONS );
    mVideoView1.SetProperty( Actor::Property::SIZE, Vector2( INIT_WIDTH, INIT_HEIGHT ) );
    mVideoView1.SetProperty( VideoView::Property::LOOPING, true );
    mVideoView1.SetProperty( VideoView::Property::MUTED, false );
    mVideoView1.SetProperty( VideoView::Property::VIDEO, PLAY_FILE2 );
    mVideoView1.SetProperty( Toolkit::VideoView::Property::UNDERLAY, false );
    mVideoView1.Play();
#else
    TextLabel textLabel = TextLabel::New("Hello World");
    textLabel.SetProperty(Actor::Property::ANCHOR_POINT, AnchorPoint::TOP_LEFT);
    textLabel.SetProperty(Dali::Actor::Property::NAME, "helloWorldLabel");
    window.Add(textLabel);
#endif
    window.Show();
    mCurrentPosition = window.GetPosition();

#if 0
    // Create the background using the style sheet
    Control control = Control::New();
    control.SetStyleName( BACKGROUND_STYLE_NAME );
    window.Add( control );

    // Create an ImageView and add it to the window
    ImageView image = ImageView::New( IMAGE_PATH );
    image.SetProperty( Actor::Property::ANCHOR_POINT, AnchorPoint::CENTER );
    image.SetProperty( Actor::Property::PARENT_ORIGIN, Vector3( 0.5f,  0.25f, 0.5f ) );
    window.Add( image );

    // Create an ImageView with properties set from the style sheet
    ImageView styledImage = ImageView::New();
    styledImage.SetStyleName( IMAGE_STYLE_NAME );
    window.Add( styledImage );
#endif

#if 0
    mTimer = Timer::New( 5000 );
    mTimer.TickSignal().Connect(this, &Example::OnTimerTick);

    mSecondWindow = Window::New(PositionSize(0, 0, 540, 960), "", false);
    mSecondWindow.KeyEventSignal().Connect( this, &Example::OnKeyEvent );

    mSecondWindow.SetTransparency( true );
    mSecondWindow.SetBackgroundColor( Vector4( 0.9, 0.3, 0.3, 0.5) );

    Dali::Window::WindowPosition secondWindowPosition = Dali::Window::WindowPosition( 0, 0 );
    mSecondWindow.SetPosition(secondWindowPosition);

    mTextLabel2 = TextLabel::New("Second Window");
    mTextLabel2.SetProperty( Actor::Property::ANCHOR_POINT, AnchorPoint::TOP_LEFT );
    mTextLabel2.SetProperty( Actor::Property::NAME, "Second Window");

    mSecondWindow.Add( mTextLabel2 );

    //mSecondWindow.Hide();
    mSecondWindow.Show();

    mSecondWindow.AddAvailableOrientation(Dali::Window::LANDSCAPE);
    mSecondWindow.AddAvailableOrientation(Dali::Window::PORTRAIT);
    mSecondWindow.AddAvailableOrientation(Dali::Window::LANDSCAPE_INVERSE);
    mSecondWindow.AddAvailableOrientation(Dali::Window::PORTRAIT_INVERSE);
#endif
  }

  //void OnTouched(const TouchEvent& touch)
  bool  OnTouched(Actor actor, const TouchEvent& touch)
  {
    static bool touched = false;
    Window window = mApplication.GetWindow();
    if(touch.GetState(0) == PointState::DOWN)
    {
      touched = true;
      mTouchedPosition = touch.GetScreenPosition(0);
      DALI_LOG_ERROR("Touch Down, position: %f, %f\n",mTouchedPosition.x, mTouchedPosition.y);
    }
    else if(touch.GetState(0) == PointState::MOTION)
    {
      if(touched)
      {
        Vector2 diff = touch.GetScreenPosition(0) - mTouchedPosition;
        //DALI_LOG_ERROR("Touch move, old: %f, %f, new: %f, %f\n",mTouchedPosition.x, mTouchedPosition.y, touchedPosition.x, touchedPosition.y );
        //DALI_LOG_ERROR("Touch move, delta: %f, %f\n",diff.x, diff.y);

        if((fabs(diff.x) > 5.0) || (fabs(diff.y) > 5.0))
        {
          Dali::Window::WindowPosition position = Dali::Window::WindowPosition(mCurrentPosition.GetX() + diff.x, mCurrentPosition.GetY() + diff.y);
          //DALI_LOG_ERROR("Touch move, current[%f,%f], new[%d,%d]\n",mCurrentPosition.GetX(), mCurrentPosition.GetY(), position.GetX(), position.GetY());
          window.SetPosition(position);
          mCurrentPosition = window.GetPosition();
          mTouchedPosition = touch.GetScreenPosition(0);

        }
      }
    }
    else if(touch.GetState(0) == PointState::UP)
    {
      DALI_LOG_ERROR("Touch Up\n");
      touched = false;
    }
    return true;
  }

  bool OnTimerTick()
  {
    DALI_LOG_ERROR("OnTimerTick()!!! time out\n");

    Dali::Window winHandle = mApplication.GetWindow();
    winHandle.Show();

    return false;
  }

  ///< Called when a key is pressed, we'll use this to quit
  void OnKeyEvent( const KeyEvent& event )
  {
    if( event.GetState() == KeyEvent::DOWN )
    {
      DALI_LOG_ERROR("OnKeyEvent()!!!\n");
      if ( IsKey( event, Dali::DALI_KEY_ESCAPE ) || IsKey( event, Dali::DALI_KEY_BACK ) )
      {
        mApplication.Quit();
      }
      else if (event.GetKeyName()  == "1")
      {
        DALI_LOG_ERROR("OnKeyEvent()!!! pressed 1 button(100x100)\n");
        Window window = mApplication.GetWindow();
        //DevelWindow::SetPositionSize( window, PositionSize( 0, 0, 100, 100 ));
        window.Hide();
        mTimer.Start();
      }
#if 0
      else if (event.GetKeyName()  == "2")
      {
        DALI_LOG_ERROR("OnKeyEvent()!!! pressed 2 button(200x200)\n");
        Window window = mApplication.GetWindow();
        DevelWindow::SetPositionSize( window, PositionSize( 0, 0, 200, 200 ));
      }
      else if (event.GetKeyName()  == "3")
      {
        DALI_LOG_ERROR("OnKeyEvent()!!! pressed 3 button(seconde window hide)\n");
        mSecondWindow.Hide();
      }
      else if (event.GetKeyName()  == "4")
      {
        DALI_LOG_ERROR("OnKeyEvent()!!! pressed 4 button(seconde window show)\n");
        mSecondWindow.Show();
      }
#endif
    }
  }

private:
  Application&  mApplication;
  Dali::Window mSecondWindow;
  TextLabel mTextLabel2;
  Timer mTimer;
  Dali::Window::WindowPosition mCurrentPosition;
  Vector2 mTouchedPosition;

  VideoView     mVideoView1;
};

int DALI_EXPORT_API main( int argc, char **argv )
{
  Application application = Application::New( &argc, &argv, STYLE_PATH );
  Example test( application );
  application.MainLoop();
  return 0;
}
