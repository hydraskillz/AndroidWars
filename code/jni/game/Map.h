#pragma once

namespace mage
{
	class Tile
	{
	public:
		Tile();
		Tile( const Tile& other );
		~Tile();

		void operator=( const Tile& other );

		void SetTerrainType( TerrainType* terrainType );
		void ClearTerrainType();
		TerrainType* GetTerrainType() const;
		bool HasTerrainType() const;

		void SetOwner( Faction* player );
		void ClearOwner();
		Faction* GetOwner() const;
		bool HasOwner() const;

		Unit* GetUnit() const;
		bool IsOccupied() const;
		bool IsEmpty() const;

		bool IsCapturable() const;

		void Open( int searchIndex );
		bool IsOpen( int searchIndex ) const;
		void Close( int searchIndex );
		bool IsClosed( int searchIndex ) const;

		void SetPreviousTileDirection( PrimaryDirection direction );
		PrimaryDirection GetPreviousTileDirection() const;

		void SetBestTotalCostToEnter( int totalCostToEnter );
		int GetBestTotalCostToEnter() const;

	private:
		void SetUnit( Unit* unit );
		void ClearUnit();

		PrimaryDirection mPreviousTileDirection;
		int mBestTotalCostToEnter;
		int mLastOpenedSearchIndex;
		int mLastClosedSearchIndex;
		TerrainType* mTerrainType;
		Faction* mOwner;
		Unit* mUnit;

	public:
		Event<> OnChanged;

		friend class Map;
		friend class Unit;
	};


	/**
	 * Holds information about each tile on the map and allows tile data to be manipulated.
	 */
	class Map : public Grid< Tile, MAP_SIZE_POWER_OF_TWO >
	{
	public:
		static const char* const MAPS_FOLDER_PATH;
		static const char* const MAP_FILE_EXTENSION;

		typedef std::vector< Faction* > Factions;
		typedef std::vector< Unit* > Units;
		typedef std::vector< Iterator > Tiles;
		typedef std::set< Iterator > TileSet;

		typedef Delegate< void, const Map::Iterator& > OnTileChangedCallback;

		typedef Delegate< void, Player* > ForEachPlayerCallback;
		typedef Delegate< void, const Player* > ForEachConstPlayerCallback;
		typedef Delegate< void, Unit* > ForEachUnitCallback;
		typedef Delegate< void, const Unit* > ForEachConstUnitCallback;
		typedef Delegate< void, const Iterator&, const Unit* > ForEachReachableTileCallback;

		static std::string FormatMapPath( const std::string& mapName );

		Map();
		~Map();

		void Init( Scenario* scenario );
		void Destroy();

		void SaveToJSON( rapidjson::Document& document, rapidjson::Value& object );

		void LoadFromFile( const std::string& filePath );
		void LoadFromJSON( const rapidjson::Value& object );

		void FillWithDefaultTerrainType();

		Faction* CreateFaction();
		Faction* GetFactionByIndex( size_t index ) const;
		const Factions& GetFactions() const;
		size_t GetFactionCount() const;
		void DestroyFaction( Faction* faction );
		void DestroyAllFaction();

		void ForEachPlayer( ForEachPlayerCallback callback );
		void ForEachPlayer( ForEachConstPlayerCallback callback ) const;

		Unit* CreateUnit( UnitType* unitType, Faction* owner, short tileX, short tileY, int health = -1, int ammo = -1, int supplies = -1 );
		Unit* CreateUnit( UnitType* unitType, Faction* owner, const Vec2s& tilePos, int health = -1, int ammo = -1, int supplies = -1 );
		const Units& GetUnits() const;
		size_t GetUnitCount() const;
		void DestroyUnit( Unit* unit );
		void DestroyAllUnits();

		void ForEachUnit( ForEachUnitCallback callback );
		void ForEachUnit( ForEachConstUnitCallback callback ) const;

		void FindReachableTiles( const Unit* unit, TileSet& result );
		void ForEachReachableTile( const Unit* unit, ForEachReachableTileCallback callback );
		void FindBestPathToTile( const Unit* unit, const Vec2s& tilePos, Path& result );
		Event< Unit*, const Path& > OnUnitMoved;

		Scenario* GetScenario() const;

		int ReserveSearchIndex();

	private:
		typedef FixedSizeMinHeap< MAX_TILES, int, Iterator > OpenList;

		void TileChanged( const Iterator& tile );
		void UnitMoved( Unit* unit, const Path& path );
		void UnitDied( Unit* unit );

		bool mIsInitialized;
		int mNextSearchIndex;
		Scenario* mScenario;
		Units mUnits;
		Factions mFactions;
		OpenList mOpenList;

	public:
		Event< const Iterator& > OnTileChanged;

		friend class Tile;
		friend class Unit;
	};
}
