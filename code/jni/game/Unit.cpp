#include "androidwars.h"

using namespace mage;

MAGE_IMPLEMENT_RTTI( MapObject, Unit );

Unit::Unit( const std::string& name )
: MapObject( name )
	, mUnitType( nullptr )
	, mSprite( nullptr )
	, DrawSelected( false )
	, mOwner( nullptr )
{}


Unit::Unit( UnitType* unitType, Player* owner )
	: MapObject( "Unit" )
	, mUnitType( unitType )
	, mSprite( nullptr )
	, DrawSelected( false )
	, mOwner( owner )
{ }


Unit::~Unit() { }


/** Load the Xml properties from the MapObject */
void Unit::OnLoadProperty( const std::string& name, const std::string& value )
{
	if ( name == "UnitType" )
	{
		mUnitType = gDatabase->UnitTypes.FindByName( value );
		assertion( mUnitType, "UnitType \"%s\" not found!", value.c_str() );
	}
	else if ( name == "Owner" )
	{
		std::stringstream parser( value );

		// Read the Player index from the property.
		int index;
		parser >> index;

		// Grab the owning player.
		mOwner = gGame->GetPlayer( index );
		assertion( mOwner, "Invalid Player index %d specified for Unit \"%s\"!", mName.GetString().c_str() );
	}
}


void Unit::OnLoadFinished()
{
	// Determine the tile location of the unit.
	Vec2i tilePos = gGame->GetMap()->WorldToTile( Position );

	// Snap the unit to the tile grid.
	SetTilePos( tilePos );

	Init();
}


void Unit::Init()
{
	assertion( mUnitType != nullptr, "Unit::Init() '%s' does not have a valid UnitType!", mName.GetString().c_str() );
	assertion( mOwner != nullptr, "Unit::Init() '%s' does not have an owner Player!", mName.GetString().c_str() );

	// Create a sprite for this Unit.
	mSprite = SpriteManager::CreateSprite( mUnitType->GetAnimationSetName(), Position, "Idle" );
	BoundingRect = mSprite->GetClippingRectForCurrentAnimation();
	DebugPrintf( "Unit \"%s\" initialized!", mName.GetString().c_str() );
}


void Unit::OnDraw( const Camera& camera ) const
{
	if ( mSprite )
	{
		// Draw the sprite at the location of the Unit.
		mSprite->Position = Position;
		mSprite->OnDraw( camera );

		if ( DrawSelected )
		{
			const RectI& bounds = mSprite->GetClippingRectForCurrentAnimation();
			const Vec2f pos = Position - camera.GetPosition();
			DrawRectOutline( pos.x, pos.y, bounds.Width(), bounds.Height(), Color::CYAN, 2.0f );
		}
	}
	// Fallback to debugdraw on missing graphics
	else
	{
		MapObject::OnDraw( camera );
	}
}


void Unit::SetTilePos( const Vec2i& tilePos )
{
	// Update the tile position.
	mTilePos = tilePos;

	// Update the position of the object in the world.
	Position = gGame->GetMap()->TileToWorld( tilePos );
}


int Unit::GetMovementRange() const
{
	// Get the maximum movement range for this unit.
	UnitType* type = GetUnitType();
	int movementRange = type->GetMovementRange();

	// TODO: Take supplies into account.

	return movementRange;
}
