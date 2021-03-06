#pragma once

namespace mage
{
	class SelectUnitInputState : public DerivedInputState< GameplayState >
	{
		SelectUnitInputState( GameState* owner );
		~SelectUnitInputState();

		virtual void OnEnter( const Dictionary& parameters );
		virtual void OnExit();

		virtual bool OnPointerDown( const Pointer& pointer );
		virtual bool OnPointerUp( const Pointer& pointer );
		virtual bool OnPointerMotion( const Pointer& activePointer, const PointersByID& pointersByID );

		friend class GameState;
	};


	class MoveUnitInputState : public DerivedInputState< GameplayState >
	{
		MoveUnitInputState( GameState* owner );
		~MoveUnitInputState();

		virtual void OnEnter( const Dictionary& parameters );
		virtual void OnExit();

		virtual bool OnPointerDown( const Pointer& pointer );
		virtual bool OnPointerUp( const Pointer& pointer );
		virtual bool OnPointerMotion( const Pointer& activePointer, const PointersByID& pointersByID );

		Vec2s mLastPointerTilePos;

		friend class GameState;
	};


	class SelectActionInputState : public DerivedInputState< GameplayState >
	{
		SelectActionInputState( GameState* owner );
		~SelectActionInputState();

		virtual void OnEnter( const Dictionary& parameters );
		virtual void OnExit();

		Button* CreateActionButton( WidgetTemplate& widgetTemplate, const std::string& label );

		void OnWaitButtonPressed();
		void OnCancelButtonPressed();

		ListLayout* mActionMenu;

		friend class GameState;
	};
}
