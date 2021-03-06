#include "androidwars.h"

using namespace mage;


EditorState::EditorState() :
	//mCamera( gWindowWidth, gWindowHeight ),
	mBrushToolInputState( nullptr ),
	mPlaceToolInputState( nullptr ),
	mEraserToolInputState( nullptr ),
	mToolPalette( nullptr ),
	mTilePalette( nullptr ),
	mUnitPalette( nullptr ),
	mIsPanningCamera( false )
{ }


EditorState::~EditorState()
{
}


Map* EditorState::GetMap()
{
	return &mMap;
}


const Map* EditorState::GetMap() const
{
	return &mMap;
}


MapView* EditorState::GetMapView()
{
	return &mMapView;
}


const MapView* EditorState::GetMapView() const
{
	return &mMapView;
}


Tile EditorState::CreateTileTemplate( TerrainType* terrainType )
{
	// Create and return the Tile template.
	Tile tileTemplate;
	tileTemplate.SetTerrainType( terrainType );
	return tileTemplate;
}


Tile EditorState::CreateDefaultTileTemplate()
{
	// Get the default TerrainType for this scenario.
	TerrainType* defaultTerrainType = mScenario.GetDefaultTerrainType();

	// Create and return a Tile template for the default TerrainType.
	return CreateTileTemplate( defaultTerrainType );
}


void EditorState::PaintTileAt( float x, float y, const Tile& tile )
{
	// Find the Tile the user pressed.
	Vec2f worldCoords = mMapView.ScreenToWorldCoords( x, y );
	Vec2s tilePos = mMapView.WorldToTileCoords( worldCoords );

	if( mMap.IsValidTilePos( tilePos ) )
	{
		// If the Tile at the coordinates is valid, paint the tile.
		mMap.SetTile( tilePos, tile );
	}
}


ListLayout* EditorState::GetTilePalette() const
{
	return mTilePalette;
}


void EditorState::OnEnter( const Dictionary& parameters )
{
	// Load the default Scenario.
	// TODO: Allow this to change.
	bool success = mScenario.LoadDataFromFile( "data/Data.json" );
	assertion( success, "The Scenario file \"%s\" could not be opened!" );

	// Create a new Map and paint it with default tiles.
	mMap.Init( &mScenario );
	mMap.Resize( 16, 12 );
	mMap.FillWithDefaultTerrainType();

	// Set the default font for the MapView.
	mMapView.SetDefaultFont( gWidgetManager->GetFontByName( "default_s.fnt" ) );

	// Initialize the MapView.
	mMapView.Init( &mMap );

	// Create the tool palette Widget and show it.
	mToolPalette = gWidgetManager->CreateWidgetFromTemplate( "ToolPalette" );

	if( mToolPalette )
	{
		gWidgetManager->GetRootWidget()->AddChild( mToolPalette );
		mToolPalette->Show();

		// Bind callbacks for tool palette buttons.
		Button* brushToolButton = mToolPalette->GetChildByName< Button >( "brushToolButton" );
		Button* placeToolButton = mToolPalette->GetChildByName< Button >( "placeToolButton" );
		Button* eraserToolButton = mToolPalette->GetChildByName< Button >( "eraserToolButton" );

		if( brushToolButton )
		{
			brushToolButton->SetOnClickDelegate( [ this ]()
			{
				// Switch to the brush tool.
				DebugPrintf( "Switching to brush tool." );
				ChangeState( mBrushToolInputState );
			});
		}

		if( placeToolButton )
		{
			placeToolButton->SetOnClickDelegate( [ this ]()
			{
				// Switch to the eraser tool.
				DebugPrintf( "Switching to place tool." );
				ChangeState( mPlaceToolInputState );
			});
		}

		if( eraserToolButton )
		{
			eraserToolButton->SetOnClickDelegate( [ this ]()
			{
				// Switch to the eraser tool.
				DebugPrintf( "Switching to eraser tool." );
				ChangeState( mEraserToolInputState );
			});
		}
	}

	// Create the Tile palette Widget and hide it.
	mTilePalette = gWidgetManager->CreateWidgetFromTemplate< ListLayout >( "TilePalette" );

	if( mTilePalette )
	{
		gWidgetManager->GetRootWidget()->AddChild( mTilePalette );
		mTilePalette->Hide();

		// Add all tile type selectors to the palette.
		BuildTilePalette();
	}

	// Create the Unit palette Widget and hide it.
	mUnitPalette = gWidgetManager->CreateWidgetFromTemplate< ListLayout >( "UnitPalette" );

	if( mUnitPalette )
	{
		gWidgetManager->GetRootWidget()->AddChild( mUnitPalette );
		mUnitPalette->Hide();

		// Add all Unit type selectors to the palette.
		BuildUnitPalette();
	}

	// Create input states.
	mBrushToolInputState = CreateState< BrushToolInputState >();
	mBrushToolInputState->SetTileTemplate( CreateDefaultTileTemplate() );

	mEraserToolInputState = CreateState< EraserToolInputState >();

	// Allow the user to paint tiles.
	ChangeState( mBrushToolInputState );
}


void EditorState::OnUpdate( float elapsedTime )
{
	GameState::OnUpdate( elapsedTime );

	// Update the MapView.
	mMapView.Update( elapsedTime );
}


void EditorState::OnDraw()
{
	// Draw the MapView.
	mMapView.Draw();

	// Draw Widgets.
	GameState::OnDraw();
}


void EditorState::OnExit()
{
	// Destroy the tool palette.
	gWidgetManager->DestroyWidget( mToolPalette );

	// Destroy all states.
	DestroyState( mBrushToolInputState );

	// Destroy the Map.
	mMap.Destroy();
}


void EditorState::OnScreenSizeChanged( int32 width, int32 height )
{
	GameState::OnScreenSizeChanged( width, height );
}


bool EditorState::OnPointerDown( const Pointer& pointer )
{
	if( GetPointerCount() > 1 )
	{
		// Start translating the Camera if multi-touch.
		mIsPanningCamera = true;
	}

	return GameState::OnPointerDown( pointer );
}


bool EditorState::OnPointerUp( const Pointer& pointer )
{
	bool wasHandled = false;

	if( mIsPanningCamera )
	{
		if( GetPointerCount() == 1 )
		{
			// If the last pointer is being removed, stop panning the camera.
			mIsPanningCamera = false;
		}
	}
	else
	{
		// Otherwise, let the state handle the event.
		wasHandled = GameState::OnPointerUp( pointer );
	}

	return wasHandled;
}


bool EditorState::OnPointerMotion( const Pointer& activePointer, const PointersByID& pointersByID )
{
	bool wasHandled = false;

	if( mIsPanningCamera )
	{
		if( activePointer.isMoving )
		{
			// If multiple pointers are down, pan the Camera.
			mMapView.GetCamera()->TranslateLookAt( -activePointer.GetDisplacement() );
			wasHandled = true;
		}
	}
	else
	{
		// Otherwise, let the active state handle the event.
		wasHandled = GameState::OnPointerMotion( activePointer, pointersByID );
	}

	return wasHandled;
}


void EditorState::BuildTilePalette()
{
	if( mTilePalette )
	{
		// Clear all items in the tile palette.
		mTilePalette->DestroyAllItems();

		// Get the WidgetTemplate for the tile selector.
		WidgetTemplate* tileSelectorTemplate = gWidgetManager->GetTemplate( "TileSelector" );

		if( tileSelectorTemplate )
		{
			const TerrainTypesTable::RecordsByHashedName& terrainTypes = mScenario.TerrainTypes.GetRecords();
			for( auto it = terrainTypes.begin(); it != terrainTypes.end(); ++it )
			{
				// Add a button for each TerrainType in the Scenario.
				TerrainType* terrainType = it->second;
				Button* selector = mTilePalette->CreateItem< Button >( *tileSelectorTemplate );

				if( selector )
				{
					// Get the icon for this selector.
					Graphic* icon = selector->GetChildByName< Graphic >( "icon" );

					if( icon )
					{
						// Use the TerrainType sprite as the image for the selector.
						icon->SetSprite( terrainType->GetAnimationSetName(), "Idle" );
					}
					else
					{
						WarnFail( "Could not set icon for tile selector button \"%s\" because no \"icon\" Graphic was found!", selector->GetFullName().c_str() );
					}

					selector->SetOnClickDelegate( [ this, terrainType ]()
					{
						// Build a template tile.
						Tile tile = CreateTileTemplate( terrainType );

						// Change the selected tile type.
						mBrushToolInputState->SetTileTemplate( tile );
					});
				}
			}
		}
		else
		{
			WarnFail( "Could not build tile palette because no tile selector template was found!" );
		}
	}
}


void EditorState::BuildUnitPalette()
{
	if( mUnitPalette )
	{
		// Clear all items in the Unit palette.
		mUnitPalette->DestroyAllItems();

		// Get the WidgetTemplate for the tile selector.
		WidgetTemplate* unitSelectorTemplate = gWidgetManager->GetTemplate( "UnitSelector" );

		if( unitSelectorTemplate )
		{
			const UnitTypesTable::RecordsByHashedName& unitTypes = mScenario.UnitTypes.GetRecords();
			for( auto it = unitTypes.begin(); it != unitTypes.end(); ++it )
			{
				// Add a button for each TerrainType in the Scenario.
				UnitType* unitType = it->second;
				Button* selector = mUnitPalette->CreateItem< Button >( *unitSelectorTemplate );

				if( selector )
				{
					// Get the icon for this selector.
					Graphic* icon = selector->GetChildByName< Graphic >( "icon" );

					if( icon )
					{
						// Use the TerrainType sprite as the image for the selector.
						icon->SetSprite( unitType->GetAnimationSetName(), "Idle" );
					}
					else
					{
						WarnFail( "Could not set icon for Unit selector button \"%s\" because no \"icon\" Graphic was found!", selector->GetFullName().c_str() );
					}

					selector->SetOnClickDelegate( [ this, unitType ]()
					{
						// Change the selected Unit type.
						mPlaceToolInputState->SetSelectedUnitType( unitType );
					});
				}
			}
		}
		else
		{
			WarnFail( "Could not build tile palette because no tile selector template was found!" );
		}
	}
}
