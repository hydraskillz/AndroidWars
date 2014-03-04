#include "androidwars.h"

using namespace mage;


GameState::GameState() :
	mManager( nullptr ),
	mActiveState( nullptr ),
	mHasPendingStateChange( false ),
	mDefaultCamera( gWindowWidth, gWindowHeight )
{ }


GameState::~GameState()
{
	// Make sure the exit code was run before deleting the state.
	assertion( !IsInitialized(), "Cannot delete GameState because it is still initialized!" );
}


void GameState::Enter( GameStateManager* manager, const Dictionary& parameters )
{
	assertion( !IsInitialized(), "Cannot enter GameState because it has already been initialized!" );

	// Keep track of the manager for this state.
	mManager = manager;
	assertion( mManager, "Cannot enter state because no GameStateManager was provided!" );

	// Initialize the state.
	OnEnter( parameters );
}


void GameState::Update( float elapsedTime )
{
	if( HasPendingStateChange() )
	{
		if( mActiveState != nullptr )
		{
			// If there is a pending state change and an active InputState,
			// exit the current InputState (if any).
			mActiveState->Exit();
			mActiveState = nullptr;
		}

		// Switch in the pending state.
		mActiveState = mPendingState;
		mPendingState = nullptr;
		mHasPendingStateChange = false;

		if( mActiveState != nullptr )
		{
			// Enter the active InputState.
			mActiveState->Enter( mPendingStateParameters );
		}
	}

	// Run the state update code.
	OnUpdate( elapsedTime );

	if( mActiveState )
	{
		// Let the current InputState update itself.
		mActiveState->Update( elapsedTime );
	}
}


void GameState::OnUpdate( float elapsedTime )
{
	// Update the WidgetManager.
	gWidgetManager->Update( elapsedTime );
}


void GameState::Draw()
{
	// Run the state draw code.
	OnDraw();

	if( mActiveState )
	{
		// Let the current InputState draw itself.
		mActiveState->Draw();
	}
}


void GameState::OnDraw()
{
	// By default, draw all Widgets.
	gWidgetManager->Draw( mDefaultCamera );
}


void GameState::Exit()
{
	assertion( IsInitialized(), "Cannot exit GameState because it is not currently initialized!" );

	if( mActiveState )
	{
		// If there is an active InputState, switch it out and let it clean itself up.
		mActiveState->Exit();
		mActiveState = nullptr;
	}

	// Run the exit code.
	OnExit();

	// Unregister the manager.
	mManager = nullptr;
}


void GameState::OnScreenSizeChanged( int32 width, int32 height )
{
	// TODO
};


bool GameState::OnPointerDown( float x, float y, size_t which )
{
	bool wasHandled = false;

	if( !gWidgetManager->PointerDown( x, y, which ) && mActiveState )
	{
		// Let the current InputState handle the event.
		wasHandled = mActiveState->OnPointerDown( x, y, which );
	}

	return wasHandled;
};


bool GameState::OnPointerUp( float x, float y, size_t which )
{
	bool wasHandled = false;

	if( !gWidgetManager->PointerUp( x, y, which ) && mActiveState )
	{
		// Let the current InputState handle the event.
		wasHandled = mActiveState->OnPointerUp( x, y, which );
	}

	return wasHandled;
};


bool GameState::OnPointerMotion( float x, float y, float dx, float dy, size_t which )
{
	// Let the Widget manager handle the event first.
	bool wasHandled = false;

	if( mActiveState )
	{
		// Let the current InputState handle the event.
		wasHandled = mActiveState->OnPointerMotion( x, y, dx, dy, which );
	}

	return wasHandled;
};


void GameState::ChangeState( InputState* inputState, const Dictionary& parameters )
{
	// Cancel any pending state change.
	CancelStateChange();

	if( inputState )
	{
		assertion( inputState->GetOwner() == this, "Cannot change to InputState that was not created by the current GameState!" );

		// Keep track of the new InputState as the pending state.
		mPendingState = inputState;
		mPendingStateParameters = parameters;
		mHasPendingStateChange = true;
	}
}


void GameState::CancelStateChange()
{
	// Clear the pending state.
	mPendingState = nullptr;
	mHasPendingStateChange = false;
}


void GameState::DestroyState( InputState* inputState )
{
	assertion( inputState, "Cannot destroy null InputState!" );
	assertion( inputState->GetOwner() == this, "Cannot destroy InputState that was not created by the current GameState!" );
	assertion( !inputState->IsInitialized(), "Cannot destroy InputState that is currently initialized!" );

	// Destroy the InputState.
	delete inputState;
}

