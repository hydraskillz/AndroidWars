#include "androidwars.h"

using namespace mage;


const char* const Map::MAPS_FOLDER_PATH = "map";
const char* const Map::MAP_FILE_EXTENSION = "maps/";


Tile::Tile() :
	mTerrainType( nullptr ),
	mOwner( nullptr ),
	mUnit( nullptr )
{ }


Tile::Tile( const Tile& other ) :
	mTerrainType( other.mTerrainType ),
	mOwner( other.mOwner ),
	OnChanged() // Don't copy event bindings.
{ }


Tile::~Tile() { }


void Tile::operator=( const Tile& other )
{
	// Set all properties from the other Tile.
	SetTerrainType( other.mTerrainType );
	SetOwner( other.mOwner );
}


void Tile::SetTerrainType( TerrainType* terrainType )
{
	// Save the old value.
	TerrainType* oldTerrainType = mTerrainType;

	// Set the TerrainType.
	mTerrainType = terrainType;

	if( !IsCapturable() )
	{
		// If the tile should not have an owner, clear it.
		ClearOwner();
	}

	if( mTerrainType != oldTerrainType )
	{
		// If the TerrainType changed, fire the changed callback.
		OnChanged.Invoke();
	}
}


void Tile::ClearTerrainType()
{
	SetTerrainType( nullptr );
}


TerrainType* Tile::GetTerrainType() const
{
	return mTerrainType;
}


bool Tile::HasTerrainType() const
{
	return ( mTerrainType != nullptr );
}


void Tile::SetOwner( Faction* owner )
{
	// Save the old value.
	Faction* oldOwner = mOwner;

	if( !owner || IsCapturable() )
	{
		// Only set the owner if the tile can be captured or the
		// owner is being cleared.
		mOwner = owner;
	}

	if( mOwner != oldOwner )
	{
		// If the owner changed, fire the changed callback.
		OnChanged.Invoke();
	}
}


void Tile::ClearOwner()
{
	SetOwner( nullptr );
}


Faction* Tile::GetOwner() const
{
	return mOwner;
}


bool Tile::HasOwner() const
{
	return ( mOwner != nullptr );
}


void Tile::SetUnit( Unit* unit )
{
	mUnit = unit;
}


void Tile::ClearUnit()
{
	SetUnit( nullptr );
}


Unit* Tile::GetUnit() const
{
	return mUnit;
}


bool Tile::IsEmpty() const
{
	return ( mUnit == nullptr );
}


bool Tile::IsOccupied() const
{
	return ( mUnit != nullptr );
}


bool Tile::IsCapturable() const
{
	return ( mTerrainType && mTerrainType->IsCapturable() );
}


std::string Map::FormatMapPath( const std::string& mapName )
{
	std::stringstream formatter;
	formatter << MAPS_FOLDER_PATH << "/" << mapName << "." << MAP_FILE_EXTENSION;
	return formatter.str();
}


Map::Map() :
	mIsInitialized( false ),
	mScenario( nullptr ),
	mNextPathIndex( 0 )
{ }


Map::~Map() { }


void Map::Init( Scenario* scenario )
{
	assertion( !mIsInitialized, "Cannot initialize Map that has already been initialized!" );

	// Initialize the Map.
	mIsInitialized = true;

	// Set the Scenario.
	assertion( scenario, "Cannot initialize Map without a valid Scenario!" );
	mScenario = scenario;

	// Make sure the size of the map is valid.
	assertion( IsValid(), "Cannot initialize Map with invalid size (%d,%d)!", GetWidth(), GetHeight() );

	// Get the default TerrainType for the Scenario.
	TerrainType* defaultTerrainType = mScenario->GetDefaultTerrainType();

	ForEachTileInMaxArea( [ this, defaultTerrainType ]( const Iterator& tile )
	{
		// Initialize all tiles to the default TerrainType.
		tile->SetTerrainType( defaultTerrainType );

		tile->OnChanged.AddCallback( [ this, tile ]()
		{
			// When the Tile changes, notify the Map that the Tile has changed.
			TileChanged( tile );
		});
	});
}


void Map::Destroy()
{
	assertion( mIsInitialized, "Cannot destroy Map that has not been initialized!" );

	// Clear the scenario.
	mScenario = nullptr;

	// Destroy the Map.
	mIsInitialized = false;
}


void Map::SaveToJSON( rapidjson::Document& document, rapidjson::Value& object )
{
	DebugPrintf( "Saving game state..." );

	// TODO: Add the ID.
	//rapidjson::Value gameIDValue;
	//gameIDValue.SetString( mGameID.c_str(), result.GetAllocator() );
	//result.AddMember( "id", gameIDValue, result.GetAllocator() );

	// Start an array for Units.
	rapidjson::Value unitsArray;
	unitsArray.SetArray();
	DebugPrintf( "Created array!" );

	ForEachUnit( [ &document, &object, &unitsArray ]( Unit* unit )
	{
		// Serialize each Unit.
		rapidjson::Value unitJSON;
		unitJSON.SetObject();

		// Have the Unit serialize its values to JSON.
		unit->SaveToJSON( document, unitJSON );

		// Add each Unit's JSON to the array.
		unitsArray.PushBack( unitJSON, document.GetAllocator() );
	});

	// Add it to the result.
	object.AddMember( "units", unitsArray, document.GetAllocator() );
	DebugPrintf( "Added array!" );
}


void Map::LoadFromFile( const std::string& filePath )
{
	// TODO
}


void Map::LoadFromJSON( const rapidjson::Value& object )
{
	// Destroy all Units.
	// TODO: Don't do this.
	DestroyAllUnits();

	// Get the Units tag.
	const rapidjson::Value& unitsArray = object[ "units" ];
	assertion( unitsArray.IsArray(), "Could not load game state from JSON because no \"units\" list was found!" );

	Scenario* scenario = GetScenario();

	for( auto it = unitsArray.Begin(); it != unitsArray.End(); ++it )
	{
		const rapidjson::Value& object = ( *it );
		assertion( object.IsObject(), "Could not load Unit from JSON because the JSON provided was not an object!" );

		// Get all properties.
		HashString unitTypeName = GetJSONStringValue( object, "unitType", "" );
		int ownerIndex = GetJSONIntValue( object, "owner", -1 );
		int tileX = GetJSONIntValue( object, "x", -1 );
		int tileY = GetJSONIntValue( object, "y", -1 );

		assertion( GetTile( tileX, tileY ).IsValid(), "Loaded invalid tile position (%d,%d) from JSON!", tileX, tileY );

		// Get references.
		UnitType* unitType = scenario->UnitTypes.FindByName( unitTypeName );
		assertion( unitType, "Could not load invalid UnitType (\"%s\") from JSON!", unitTypeName.GetCString() );

		Faction* faction = GetFactionByIndex( ownerIndex );
		assertion( faction, "Could not load Unit with invalid Faction index (%d) from JSON!", ownerIndex );

		// Spawn the Unit.
		Unit* unit = CreateUnit( unitType, faction, tileX, tileY );

		// Load each Unit from the array.
		unit->LoadFromJSON( *it );
	}
}


void Map::FillWithDefaultTerrainType()
{
	// Get the default TerrainType for this Scenario.
	TerrainType* defaultTerrainType = mScenario->GetDefaultTerrainType();
	assertion( defaultTerrainType != nullptr, "No default TerrainType found for this Scenario!" );

	// Fill the whole Map with the default TerrainType.
	Tile tile;
	tile.SetTerrainType( defaultTerrainType );
	FillMaxArea( tile );
}


Faction* Map::CreateFaction()
{
	// Create a new Faction.
	Faction* faction = new Faction( this );

	// Add the Faction to the list of Factions.
	mFactions.push_back( faction );

	return faction;
}


Faction* Map::GetFactionByIndex( size_t index ) const
{
	Faction* faction = nullptr;

	if( index >= 0 && index < mFactions.size() )
	{
		faction = mFactions[ index ];
	}

	return faction;
}


const Map::Factions& Map::GetFactions() const
{
	return mFactions;
}


size_t Map::GetFactionCount() const
{
	return mFactions.size();
}


void Map::DestroyFaction( Faction* faction )
{
	assertion( faction->GetMap() == this, "Cannot destroy Faction created by a different Map!" );

	auto it = mFactions.begin();

	for( ; it != mFactions.end(); ++it )
	{
		if( *it == faction )
		{
			// Find the Faction in the list of Factions.
			break;
		}
	}

	// Remove the Faction from the list of Factions.
	assertion( it != mFactions.end(), "Cannot destroy Faction because it was not found in the Map Faction list!" );
	mFactions.erase( it );

	// Destroy the Faction.
	delete faction;
}


Unit* Map::CreateUnit( UnitType* unitType, Faction* owner, short tileX, short tileY, int health, int ammo )
{
	return CreateUnit( unitType, owner, Vec2s( tileX, tileY ) );
}


Unit* Map::CreateUnit( UnitType* unitType, Faction* owner, const Vec2s& tilePos, int health, int ammo )
{
	// Create a new Unit.
	Unit* unit = new Unit();

	// Load Unit properties.
	unit->SetUnitType( unitType );
	unit->SetOwner( owner );

	// Get the Tile where the Unit will be placed.
	Iterator tile = GetTile( tilePos );
	assertion( tile.IsValid(), "Cannot create Unit at invalid Tile (%d,%d)!", tilePos.x, tilePos.y );
	assertion( tile->IsEmpty(), "Cannot create Unit at Tile (%d,%d) because the Tile is occupied by another Unit!", tilePos.x, tilePos.y );

	// Set the health and ammo for the Unit.
	if( health >= 0 )
	{
		unit->SetHealth( health );
	}

	if( ammo >= 0 )
	{
		unit->SetAmmo( ammo );
	}

	// Initialize the Unit.
	unit->Init( this, tile );

	// Place the Unit into the Tile.
	tile->SetUnit( unit );

	// Add the Unit to the list of Units.
	mUnits.push_back( unit );

	return unit;
}


void Map::ForEachUnit( ForEachUnitCallback callback )
{
	for( auto it = mUnits.begin(); it != mUnits.end(); ++it )
	{
		// Call the function for each Unit.
		callback.Invoke( *it );
	}
}


void Map::ForEachUnit( ForEachConstUnitCallback callback ) const
{
	for( auto it = mUnits.begin(); it != mUnits.end(); ++it )
	{
		// Call the function for each Unit.
		callback.Invoke( *it );
	}
}


const Map::Units& Map::GetUnits() const
{
	return mUnits;
}


size_t Map::GetUnitCount() const
{
	return mUnits.size();
}


void Map::DestroyUnit( Unit* unit )
{
	// TODO
}


void Map::DestroyAllUnits()
{
	// TODO
}


Scenario* Map::GetScenario() const
{
	return mScenario;
}


void Map::TileChanged( const Iterator& tile )
{
	// Fire the change callback.
	OnTileChanged.Invoke( tile );
}


void Map::UnitMoved( Unit* unit, const Path& path )
{
	// TODO
}


void Map::UnitDied( Unit* unit )
{
	// TODO
}
