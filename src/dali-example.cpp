/*
 * Copyright (c) 2019 Samsung Electronics Co., Ltd.
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

#include <dali-toolkit/dali-toolkit.h>
#include <dali/integration-api/scene.h>
#include <dali/devel-api/adaptor-framework/window-devel.h>
#include <dali/integration-api/debug.h>
#include <iostream>

using namespace Dali;
using namespace Dali::Toolkit;

// This example shows how to create and display Hello World! using a simple TextActor
//
class HelloWorldController : public ConnectionTracker
{
public:

  HelloWorldController( Application& application )
  : mApplication( application ),
    rotation_flag( 0 ),
    rotation_count( 0 ),
    rot( 0 )
  {
    // Connect to the Application's Init signal
    mApplication.InitSignal().Connect( this, &HelloWorldController::Create );
  }

  ~HelloWorldController() = default;

  // The Init signal is received once (only) during the Application lifetime
  void Create( Application& application )
  {
    // Get a handle to the window and set the background colour
    Window window = application.GetWindow();
    window.SetType(WindowType::IME);
    window.SetBackgroundColor( Color::WHITE );

#if 0
    TextLabel textLabel = TextLabel::New("Hello World");
    textLabel.SetProperty(Actor::Property::ANCHOR_POINT, AnchorPoint::TOP_LEFT);
    textLabel.SetProperty(Dali::Actor::Property::NAME, "helloWorldLabel");
    window.Add(textLabel);
#else
    // Add a text label to the window
    TextLabel textLabel = TextLabel::New( "Hello World" );
    textLabel.SetProperty( Actor::Property::ANCHOR_POINT, AnchorPoint::CENTER );
    textLabel.SetProperty( Actor::Property::PARENT_ORIGIN, ParentOrigin::CENTER );
    textLabel.SetProperty( Actor::Property::NAME, "helloWorldLabel" );
    textLabel.SetBackgroundColor( Color::RED );
    window.Add( textLabel );

    // Respond to key events
    window.KeyEventSignal().Connect( this, &HelloWorldController::OnKeyEvent );

    // Create a clipping control and add a child to it
    mClipControl = Control::New();
    mClipControl.SetProperty( Actor::Property::SIZE, Vector2( 250.0f, 100.0f ) );
    mClipControl.SetProperty( Actor::Property::PARENT_ORIGIN, Vector3(  0.75f, 0.75f, 0.5f)  );
    mClipControl.SetProperty( Actor::Property::ANCHOR_POINT, AnchorPoint::CENTER  );
    mClipControl.SetBackgroundColor( Color::BLUE );
    window.Add( mClipControl );

    auto child = Control::New();
    child.SetProperty( Actor::Property::SIZE, Vector2( 100.0f, 250.0f  ) );
    child.SetProperty( Actor::Property::PARENT_ORIGIN, AnchorPoint::CENTER  );
    child.SetProperty( Actor::Property::ANCHOR_POINT, AnchorPoint::CENTER  );
    child.SetBackgroundColor( Color::GREEN );
    mClipControl.Add( child );
#endif
    Dali::Window winHandle = application.GetWindow();
    winHandle.AddAvailableOrientation( Dali::WindowOrientation::PORTRAIT );
    winHandle.AddAvailableOrientation( Dali::WindowOrientation::LANDSCAPE );
    winHandle.AddAvailableOrientation( Dali::WindowOrientation::PORTRAIT_INVERSE );
    winHandle.AddAvailableOrientation( Dali::WindowOrientation::LANDSCAPE_INVERSE );

    DevelWindow::SetPositionSize( window, PositionSize( 0, 0, 720, 442 ));

    DevelWindow::SetPartialWindowOrientation(winHandle, Dali::WindowOrientation::PORTRAIT, PositionSize(0, 0, 720, 442));
    DevelWindow::SetPartialWindowOrientation(winHandle, Dali::WindowOrientation::LANDSCAPE, PositionSize(0, 0, 318, 1280));
    DevelWindow::SetPartialWindowOrientation(winHandle, Dali::WindowOrientation::PORTRAIT_INVERSE, PositionSize(0, 0, 720, 442));
    DevelWindow::SetPartialWindowOrientation(winHandle, Dali::WindowOrientation::LANDSCAPE_INVERSE, PositionSize(0, 0, 318, 1280));

//      DevelWindow::SetPositionSize( window, PositionSize( 0, 0, 720, 300 ));

//      DevelWindow::SetPartialWindowOrientation(winHandle, Dali::WindowOrientation::PORTRAIT, PositionSize(0, 0, 720, 300));
//      DevelWindow::SetPartialWindowOrientation(winHandle, Dali::WindowOrientation::LANDSCAPE, PositionSize(420, 0, 300, 1280));
//      DevelWindow::SetPartialWindowOrientation(winHandle, Dali::WindowOrientation::PORTRAIT_INVERSE, PositionSize(0, 0, 720, 300));
//      DevelWindow::SetPartialWindowOrientation(winHandle, Dali::WindowOrientation::LANDSCAPE_INVERSE, PositionSize(0, 0, 1280, 300));

    winHandle.Show();
  }

  bool OnTimerTick()
  {
    DALI_LOG_ERROR("OnTimerTick()!!! time out\n");
    Dali::Window winHandle = mApplication.GetWindow();
    winHandle.Show();
    return false;
  }

  void OnKeyEvent( const KeyEvent& event )
  {
#if 1
      if( event.GetState() == KeyEvent::DOWN )
      {
        std::cout << "KeyName: " << event.GetKeyName() << std::endl;
        if (event.GetKeyName()  == "1")
        {
            // Toggle the clipping mode on mClippingControl if any other actor by pressing any key
            ClippingMode::Type currentMode;
            mClipControl.GetProperty( Actor::Property::CLIPPING_MODE ).Get( currentMode );
            mClipControl.SetProperty(
                Actor::Property::CLIPPING_MODE,
                ( ( currentMode == ClippingMode::DISABLED ) ? ClippingMode::CLIP_TO_BOUNDING_BOX : ClippingMode::DISABLED ) );
        }
        else if (event.GetKeyName()  == "2")
        {
            Dali::Window winHandle = mApplication.GetWindow();
            mTimer = Timer::New( 5000 );
            mTimer.TickSignal().Connect(this, &HelloWorldController::OnTimerTick);
            mTimer.Start();
            winHandle.Hide();
        }
        else if (event.GetKeyName()  == "3")
        {
            mSecondWindow = Window::New(PositionSize(0, 0, 1920, 1080), "", false);
#if 0
            mSecondWindow.SetTransparency( true );
            mSecondWindow.SetBackgroundColor( Vector4( 0.9, 0.3, 0.3, 0.5) );

            Dali::Window::WindowPosition secondWindowPosition = Dali::Window::WindowPosition( 0, 0 );
            mSecondWindow.SetPosition(secondWindowPosition);

            mTextLabel2 = TextLabel::New( "Second window" );
            mTextLabel2.SetProperty( Actor::Property::ANCHOR_POINT, AnchorPoint::TOP_LEFT );
            mTextLabel2.SetProperty( Actor::Property::NAME, "Second Window");

            mSecondWindow.Add( mTextLabel2 );

            mSecondWindow.AddAvailableOrientation( Dali::Window::LANDSCAPE );
            mSecondWindow.AddAvailableOrientation( Dali::Window::PORTRAIT );
            mSecondWindow.AddAvailableOrientation( Dali::Window::LANDSCAPE_INVERSE );
            mSecondWindow.AddAvailableOrientation( Dali::Window::PORTRAIT_INVERSE );

            //mSecondWindow.Hide();
            mSecondWindow.Show();
#endif
            mSecondWindow.Hide();
            //mClipControl.SetProperty( Actor::Property::PARENT_ORIGIN, Vector3(  0.25f, 0.25f, 0.5f)  );
        }
      }
#endif
  }

private:
  Application&  mApplication;
  Control mClipControl;
  int rotation_flag;
  int rotation_count;
  int rot;
  Timer mTimer;
  TextLabel mTextLabel2;

  Dali::Window mSecondWindow;
};

int DALI_EXPORT_API main( int argc, char **argv )
{
  Application application = Application::New( &argc, &argv );
  HelloWorldController test( application );
  application.MainLoop();
  return 0;
}
