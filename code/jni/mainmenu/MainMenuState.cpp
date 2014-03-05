#include "androidwars.h"

using namespace mage;


const Vec2f MainMenuState::BACKGROUND_SCROLL_VELOCITY( -100.0f, 50.0f );


MainMenuState::MainMenuState() :
	GameState(),
	mLogInState( nullptr ),
	mDashboardState( nullptr ),
	mWidget( nullptr )
{
	DebugPrintf( "MainMenuState created!" );
}


MainMenuState::~MainMenuState()
{
	DebugPrintf( "MainMenuState destroyed!" );
}


void MainMenuState::OnEnter( const Dictionary& parameters )
{
	DebugPrintf( "MainMenuState entered!" );

	// Create the main menu widget.
	mWidget = gWidgetManager->CreateWidgetFromTemplate( "MainMenu" );

	if( mWidget )
	{
		// Show the main menu widget.
		gWidgetManager->GetRootWidget()->AddChild( mWidget );
		mWidget->Show();
	}

	// Create all states.
	mLogInState = CreateState< LogInInputState >();
	mDashboardState = CreateState< DashboardInputState >();

	if( !gOnlineGameClient->IsAuthenticated() )
	{
		// If no user is currently logged in, show the login screen.
		ChangeState( mLogInState );
	}
	else
	{
		// Otherwise, show the user dashboard.
		ChangeState( mDashboardState );
	}
}


void MainMenuState::OnUpdate( float elapsedTime )
{
	GameState::OnUpdate( elapsedTime );

	if( mWidget )
	{
		// Get the scrolling background.
		Graphic* scrollBackground = mWidget->GetChildByName< Graphic >( "scrollBackground" );

		if( scrollBackground )
		{
			// Update the scroll of the scrolling background.
			Vec2f scrollDistance( scrollBackground->GetDrawOffset() + BACKGROUND_SCROLL_VELOCITY * elapsedTime );
			scrollBackground->SetDrawOffset( scrollDistance );
		}
	}
}


void MainMenuState::OnDraw()
{
	GameState::OnDraw();
}


void MainMenuState::OnExit()
{
	DebugPrintf( "MainMenuState exited!" );

	// Destroy all states.
	DestroyState( mLogInState );
	DestroyState( mDashboardState );
}


LogInInputState* MainMenuState::GetLogInState() const
{
	return mLogInState;
}


DashboardInputState* MainMenuState::GetDashboardState() const
{
	return mDashboardState;
}


Widget* MainMenuState::GetWidget() const
{
	return mWidget;
}


// ========== LogInInputState ==========

LogInInputState::LogInInputState( GameState* owner ) :
	DerivedInputState( owner ),
	mProgressDialog( nullptr )
{ }


LogInInputState::~LogInInputState()
{
}


void LogInInputState::OnEnter( const Dictionary& parameters )
{
	DebugPrintf( "Entering LogInInputState" );

	Widget* loginScreen = GetLoginScreen();

	if( loginScreen )
	{
		// Get Login button.
		Button* loginButton = loginScreen->GetChildByName< Button >( "loginButton" );

		if( loginButton )
		{
			// Register callbacks.
			loginButton->SetOnClickDelegate( [this]( float x, float y )
			{
				OnLogInButtonPressed( x, y );
			});
		}

		// Show the login screen.
		loginScreen->Show();
	}
}


void LogInInputState::OnExit()
{
	DebugPrintf( "Exiting LogInInputState" );

	Widget* loginScreen = GetLoginScreen();

	if( loginScreen )
	{
		// Get Login button.
		Button* loginButton = loginScreen->GetChildByName< Button >( "loginButton" );

		if( loginButton )
		{
			// Unregister callbacks.
			loginButton->ClearOnClickDelegate();
		}

		// Hide the login screen.
		loginScreen->Hide();
	}
}


void LogInInputState::OnLogInButtonPressed( float x, float y )
{
	DebugPrintf( "Log in button pressed!" );

	MainMenuState* owner = GetOwnerDerived();
	Widget* loginScreen = GetLoginScreen();

	if( loginScreen )
	{
		// Get the username and password values from the login box.
		TextField* usernameField = loginScreen->GetChildByName< TextField >( "usernameField" );
		TextField* passwordField = loginScreen->GetChildByName< TextField >( "passwordField" );

		if( usernameField && passwordField )
		{
			std::string username = usernameField->GetText();
			std::string password = passwordField->GetText();

			// Create a progress dialog.
			mProgressDialog = owner->CreateState< ProgressInputState >();

			if( mProgressDialog )
			{
				// Specify parameters for progress dialog.
				Dictionary parameters;
				parameters.Set( "widgetName", std::string( "progressDialog" ) );
				parameters.Set( "template", std::string( "Progress" ) );

				// Show the progress dialog.
				owner->PushState( mProgressDialog, parameters );
			}
			else
			{
				WarnFail( "Could not create progress dialog!" );
			}

			// Fire off the login request to the server.
			gOnlineGameClient->LogIn( username, password, [this]( bool success )
			{
				MainMenuState* owner = GetOwnerDerived();

				if( success )
				{
					DebugPrintf( "Login successful!" );

					// If the login was successful, go to the dashboard.
					owner->ChangeState( owner->GetDashboardState() );
				}
				else
				{
					// TODO: If the login failed, give the user feedback.
					DebugPrintf( "Login failed!" );

					if( mProgressDialog )
					{
						// Notify the dialog that it should close.
					}
				}
			});
		}
	}
}


Widget* LogInInputState::GetLoginScreen() const
{
	Widget* loginScreen = nullptr;

	// Get the login widget.
	Widget* mainMenu = GetOwnerDerived()->GetWidget();

	if( mainMenu )
	{
		loginScreen = mainMenu->GetChildByName( "loginScreen" );
	}

	return loginScreen;
}


// ========== DashboardInputState ==========

DashboardInputState::DashboardInputState( GameState* owner ) :
	DerivedInputState( owner )
{ }


DashboardInputState::~DashboardInputState() { }


void DashboardInputState::OnEnter( const Dictionary& parameters )
{
	DebugPrintf( "Entering DashboardInputState" );

	Widget* dashboardScreen = GetDashboardScreen();

	if( dashboardScreen )
	{
		// TODO: Register callbacks.

		// Show the widget.
		dashboardScreen->Show();
	}
}


void DashboardInputState::OnExit()
{
	DebugPrintf( "Exiting DashboardInputState" );

	Widget* dashboardScreen = GetDashboardScreen();

	if( dashboardScreen )
	{
		// TODO: Register callbacks.

		// Hide the widget.
		dashboardScreen->Hide();
	}
}


ObjectEventFunc( DashboardInputState, OnLogOutButtonPressed )
{
	DebugPrintf( "Log out button pressed!" );

	// Log the current user out.
	gOnlineGameClient->LogOut();

	// If the login was successful, go to the login screen.
	MainMenuState* owner = GetOwnerDerived();
	owner->ChangeState( owner->GetLogInState() );
}


ObjectEventFunc( DashboardInputState, OnRefreshButtonPressed )
{
	DebugPrintf( "Refresh button pressed!" );

	// Request the current games list.
	gOnlineGameClient->RequestCurrentGamesList( [this]( bool success, const std::vector< OnlineGameListData >& currentGameList )
	{
		for( auto it = currentGameList.begin(); it != currentGameList.end(); ++it )
		{
			DebugPrintf( "Found game: \"%s\" (id: %s)", it->name.c_str(), it->id.c_str() );
		}
	});
}


ObjectEventFunc( DashboardInputState, OnNewGameButtonPressed )
{
	DebugPrintf( "New game button pressed!" );

	// Request a new game.
	gOnlineGameClient->CallCloudFunction( "requestMatchmakingGame", "{}" );
}


Widget* DashboardInputState::GetDashboardScreen() const
{
	Widget* dashboardScreen = nullptr;

	// Get the login widget.
	Widget* mainMenu = GetOwnerDerived()->GetWidget();

	if( mainMenu )
	{
		dashboardScreen = mainMenu->GetChildByName( "dashboardScreen" );
	}

	return dashboardScreen;
}

