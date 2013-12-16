#include "androidwars.h"

using namespace mage;


const int Unit::MAX_HP;


MAGE_IMPLEMENT_RTTI( MapObject, Unit );


Unit::Unit( const std::string& name )
: MapObject( name )
	, mUnitType( nullptr )
	, mSprite( nullptr )
	, mOwner( nullptr )
	, mHP( 0 )
	, mAP( 0 )
	, mAmmo( -1 )
{}


Unit::Unit( UnitType* unitType, Player* owner )
	: MapObject( "Unit" )
	, mUnitType( unitType )
	, mSprite( nullptr )
	, mOwner( owner )
	, mHP( 0 )
	, mAP( 0 )
	, mAmmo( -1 )
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
		// Read the Player index from the property.
		int index;
		bool success = StringUtil::StringToType( value, &index );
		assertion( success, "Could not parse Owner value \"%s\". Must be a positive integer.", value.c_str() );

		// Grab the owning player.
		mOwner = gGame->GetPlayer( index );
		assertion( mOwner, "Invalid Player index %d specified for Unit \"%s\"!", mName.GetString().c_str() );
	}
	else if ( name == "Ammo" )
	{
		// Read in ammo amount.
		int ammo;
		bool success = StringUtil::StringToType( value, &ammo );
		assertion( success, "Could not parse Ammo value! Must be a positive integer." );
		mAmmo = ammo;
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
	mSprite->DrawColor = mOwner->GetPlayerColor();
	mSelectionColor = mSprite->DrawColor;
	mDefaultColor = mSelectionColor;
	mDefaultColor.r /= 2;
	mDefaultColor.g /= 2;
	mDefaultColor.b /= 2;
	Deselect();

	// Set initial resources.
	mHP = mUnitType->GetMaxHP();

	if( mAmmo >= 0 )
	{
		// Make sure ammo value is valid.
		SetAmmo( mAmmo );
	}
	else
	{
		// If the ammo value is uninitialized, set it to the maximum amount.
		mAmmo = mUnitType->GetMaxAmmo();
	}

	mDestination = Position;

	mAP = 2;

	DebugPrintf( "Unit \"%s\" initialized!", mName.GetString().c_str() );
}


void Unit::OnDraw( const Camera& camera ) const
{
	if ( mSprite )
	{
		// Draw the sprite at the location of the Unit.
		mSprite->Position = Position;
		mSprite->OnDraw( camera );
	}
	// Fallback to debugdraw on missing graphics
	else
	{
		MapObject::OnDraw( camera );
	}

	// Draw HP
	Game* game = mOwner->GetGame();
	BitmapFont* fnt = game->GetDefaultFont();
	Vec2f textPos = Position - camera.GetPosition();
	float h = mSprite ? mSprite->GetClippingRectForCurrentAnimation().Height() / 2 : 0;
	DrawTextFormat( textPos.x, textPos.y + h - fnt->GetLineHeight(), fnt, "%d", mHP );
}


void Unit::OnUpdate( float dt )
{
	float delta = ( Position - mDestination ).LengthSqr();
	if ( delta > 1 )
	{
		Vec2f vel = mDestination - Position;
		vel.Normalize();
		vel *= 100;
		Position += vel * dt;

		delta = ( Position - mDestination ).LengthSqr();
		if ( delta < 1 )
			gGame->OnUnitReachedDestination( this );
	}
}


void Unit::SetTilePos( const Vec2i& tilePos )
{
	// Update the tile position.
	mTilePos = tilePos;

	// Update the position of the object in the world.
	Position = gGame->GetMap()->TileToWorld( tilePos );
}


void Unit::SetDestination( const Vec2i& tilePos )
{
	mDestination = gGame->GetMap()->TileToWorld( tilePos );
	mTilePos = gGame->GetMap()->WorldToTile( mDestination );
}


Player* Unit::GetOwner() const
{
	return mOwner;
}


int Unit::GetMovementRange() const
{
	// Get the maximum movement range for this unit.
	UnitType* type = GetUnitType();
	int movementRange = type->GetMovementRange();

	// TODO: Take supplies into account.

	return movementRange;
}


void Unit::Select()
{
	//mSprite->DrawColor = mSelectionColor;
	mSprite->Scale.Set( 1.15f, 1.15f );
}

void Unit::Deselect()
{
	//mSprite->DrawColor = mDefaultColor;
	mSprite->Scale.Set( 1.0f, 1.0f );
}


bool Unit::CanAttack( const Unit& target ) const
{
	const char* attackerName = mName.GetString().c_str();
	const char* targetName = target.GetName().c_str();

	DebugPrintf( "Checking whether Unit \"%s\" can attack Unit \"%s\"...", attackerName, targetName );

	// Check whether the target is in range.
	bool isInRange = IsInRange( target );
	DebugPrintf( "Unit \"%s\" %s in range.", targetName, ( isInRange ? "IS" : "IS NOT" ) );

	// Check whether this Unit can target the other Unit.
	bool canTarget = CanTarget( target );
	DebugPrintf( "Unit \"%s\" %s hit the target Unit's UnitType (%s).", attackerName, ( isInRange ? "CAN" : "CANNOT" ),
			     target.GetUnitType()->GetName().GetString().c_str() );

	bool result = ( isInRange && canTarget );
	DebugPrintf( "RESULT: Unit \"%s\" %s attack Unit \"%s\".", attackerName, ( result ? "CAN" : "CANNOT" ), targetName );

	return result;
}


void Unit::Attack( Unit& target )
{
	DebugPrintf( "Unit \"%s\" attacks Unit \"%s\"!", mName.GetString().c_str(), target.GetName().c_str() );

	// Get the best weapon to use against the target.
	int bestWeaponIndex = GetBestAvailableWeaponAgainst( target );
	assertion( bestWeaponIndex > -1, "Cannot calculate damage: No weapon can currently target that Unit!" );

	const Weapon bestWeapon = mUnitType->GetWeaponByIndex( bestWeaponIndex );
	DebugPrintf( "Best weapon: %d (\"%s\")", bestWeaponIndex, bestWeapon.GetName().GetString().c_str() );

	// Calculate damage (with randomness) and apply it to the enemy Unit.
	int damageAmount = CalculateDamageAgainst( target, bestWeaponIndex, true );
	target.TakeDamage( damageAmount );

	if( bestWeapon.ConsumesAmmo() )
	{
		// Consume ammo equal to the amount that the weapon uses per shot.
		int ammoConsumed = bestWeapon.GetAmmoPerShot();
		ConsumeAmmo( ammoConsumed );
		DebugPrintf( "Weapon consumed %d ammo. (%d ammo remaining)", ammoConsumed, mAmmo );
	}

	// TODO: Replace AP system with single move + action system.
	ConsumeAP( 1 );
}


int Unit::CalculateDamageAgainst( const Unit& target, int weaponIndex, bool calculateWithRandomness ) const
{
	int result = 0;

	// Get the best weapon to use against the target.
	const Weapon& weapon = mUnitType->GetWeaponByIndex( weaponIndex );

	DebugPrintf( "Calculating damage of Unit \"%s\" (%s) against Unit \"%s\" (%s) with weapon %d (\"%s\")...",
				 mName.GetString().c_str(), mUnitType->GetName().GetString().c_str(), target.GetName().c_str(),
				 target.GetUnitType()->GetName().GetString().c_str(), weaponIndex, weapon.GetName().GetString().c_str() );

	// Get the base amount of damage to apply.
	int baseDamagePercentage = weapon.GetDamagePercentageAgainstUnitType( target.GetUnitType() );
	assertion( baseDamagePercentage > 0, "Cannot calculate damage: weapon cannot target Unit!" );

	float baseDamageScale = ( baseDamagePercentage * 0.01f );
	DebugPrintf( "Base damage: %d%% (%f)", baseDamagePercentage, baseDamageScale );

	// Scale the damage amount based on the current health of this Unit.
	float healthScale = GetHealthScale();
	DebugPrintf( "Health scaling factor: %f", healthScale );

	// Scale the damage amount based on the target's defense bonus.
	float targetDefenseBonus = target.GetDefenseBonus();
	float targetDefenseScale = Mathf::Clamp( 1.0f - targetDefenseBonus, 0.0f, 1.0f );
	DebugPrintf( "Target defense bonus: %f (%f x damage)", targetDefenseBonus, targetDefenseScale );

	// Calculate idealized damage amount.
	float idealizedDamageAmount = ( MAX_HP * baseDamageScale * healthScale * targetDefenseScale );
	DebugPrintf( "Idealized damage amount: %f (%d x %f x %f x %f)", idealizedDamageAmount,
			     MAX_HP, baseDamageScale, healthScale, targetDefenseScale );

	// Floor the idealized damage amount to get the damage to apply.
	result = (int) idealizedDamageAmount;

	if( calculateWithRandomness )
	{
		// Determine the chance that an extra point of damage will be applied.
		int extraDamageChance = ( (int) ( idealizedDamageAmount * 10.0f ) % 10 );
		DebugPrintf( "Extra damage chance: %d in 10", extraDamageChance );

		// Roll a 10-sided die to see if the check passed.
		int extraDamageRoll = RNG::RandomInRange( 1, 10 );
		bool success = ( extraDamageChance >= extraDamageRoll );
		DebugPrintf( "Extra damage roll %s! (Rolled a %d)", ( success ? "SUCCEEDED" : "FAILED" ), extraDamageRoll );

		if( success )
		{
			// Add one point of extra damage if the extra damage roll succeeded.
			++result;
		}
	}

	DebugPrintf( "TOTAL DAMAGE: %d", result );
	return result;
}


float Unit::GetDefenseBonus() const
{
	float result;

	DebugPrintf( "Calculating defense bonus for Unit \"%s\"...", mName.GetString().c_str() );

	// Get the defensive bonus supplied by the current tile.
	TerrainType* terrainType = gGame->GetTerrainTypeOfTile( GetTilePos() );
	int coverBonus = terrainType->GetCoverBonus();
	float coverBonusScale = ( coverBonus * 0.1f );
	DebugPrintf( "Cover bonus: %d (%f)", coverBonus, coverBonusScale );

	// Scale the cover bonus by the Unit's current health.
	float healthScale = GetHealthScale();
	DebugPrintf( "Health scale: %f x cover bonus", healthScale );

	result = Mathf::Clamp( coverBonusScale * healthScale, 0.0f, 1.0f );
	DebugPrintf( "TOTAL DEFENSE BONUS: %f (%f x %f)", result, healthScale, coverBonusScale );

	return result;
}


bool Unit::CanTarget( const Unit& target ) const
{
	// Return whether this Unit has a weapon that can attack the target.
	int bestWeaponIndex = GetBestAvailableWeaponAgainst( target.GetUnitType() );
	return ( bestWeaponIndex >= 0 );
}


bool Unit::IsInRange( const Unit& target ) const
{
	UnitType* type = GetUnitType();
	const IntRange& range = type->GetAttackRange();
	DebugPrintf( "Unit pos (%d, %d) : trg pos (%d %d) d=%d r=[%d,%d]",
			mTilePos.x, mTilePos.y,
			target.mTilePos.x, target.mTilePos.y,
			GetDistanceToUnit( target ),
			range.Min, range.Max );
	return range.IsValueInRange( GetDistanceToUnit( target ) );
}


int Unit::GetDistanceToUnit( const Unit& target ) const
{
	return mTilePos.GetManhattanDistanceTo( target.mTilePos );
}


bool Unit::CanFireWeapon( int weaponIndex ) const
{
	Weapon& weapon = mUnitType->GetWeaponByIndex( weaponIndex );
	return ( !weapon.ConsumesAmmo() || ( weapon.GetAmmoPerShot() <= mAmmo ) );
}


int Unit::GetBestAvailableWeaponAgainst( const UnitType* unitType ) const
{
	assertion( unitType, "Cannot get best available weapon against NULL UnitType!" );

	int bestDamagePercentage = 0;
	int result = -1;

	DebugPrintf( "Choosing best weapon for Unit \"%s\" (%s) against UnitType \"%s\"...",
			     mName.GetString().c_str(), mUnitType->GetName().GetString().c_str(), unitType->GetName().GetString().c_str() );

	for( int i = 0; i < mUnitType->GetNumWeapons(); ++i )
	{
		// Check each weapon to see which one is the best.
		const Weapon& weapon = mUnitType->GetWeaponByIndex( i );
		int damagePercentage = weapon.GetDamagePercentageAgainstUnitType( unitType );

		bool isBestChoice = ( damagePercentage > bestDamagePercentage );
		bool canFire = CanFireWeapon( i );

		if( isBestChoice && canFire )
		{
			bestDamagePercentage = damagePercentage;
			result = i;
		}

		DebugPrintf( "Weapon %d (\"%s\") %s fire (%d%% damage)", i, weapon.GetName().GetString().c_str(),
				     ( canFire ? "CAN" : "CANNOT" ), damagePercentage );
	}

	if( result > -1 )
	{
		DebugPrintf( "BEST CHOICE: Weapon %d (\"%s\")", result, mUnitType->GetWeaponByIndex( result ).GetName().GetString().c_str() );
	}
	else
	{
		DebugPrintf( "NO WEAPON AVAILABLE!" );
	}

	return result;
}


void Unit::ResetAP()
{
	mAP = 2;
	mSprite->DrawColor = mSelectionColor;
}

void Unit::ConsumeAP( int ap )
{
	--mAP;
	if ( mAP <= 0 )
	{
		mAP = 0;
		mSprite->DrawColor = mDefaultColor;
	}
}
