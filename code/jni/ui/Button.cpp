#include <MageApp.h>
#include "Widget.h"
#include "Label.h"
#include "Button.h"

using namespace mage;

//---------------------------------------
Button::Button( const std::string& name, const XmlReader::XmlReaderIterator& itr, Widget* parent )
	: Label( name, itr, parent )
{
	mOnClickEvent = itr.GetAttributeAsString( "onClickEvent", "__DUMMY_EVENT__" );
	mOnClickAnim = itr.GetAttributeAsString( "onClickAnim", "" );
}
//---------------------------------------
Button::~Button()
{}
//---------------------------------------
bool Button::OnClick( float x, float y )
{
	if ( !Widget::OnClick( x, y ) )
	{
		// If event is inside frame, fire event
		if ( mSprite )
		{
			RectI r =  mSprite->GetClippingRectForCurrentAnimation();
			Vec2f pos = GetPosition();
			r.Left += pos.x;
			r.Top += pos.y;
			if ( r.Contains( (int) x, (int) y ) )
			{
				mSprite->PlayAnimation( mOnClickAnim );
				EventManager::FireEvent( mOnClickEvent );
				return true;
			}
		}
	}
	return false;
}
//---------------------------------------