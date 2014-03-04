#pragma once

namespace mage
{
	class AbstractWidgetFactory;
	class Widget;


	class WidgetManager
	{
	public:
		WidgetManager();
		virtual ~WidgetManager();

		void Init();
		void Destroy();

		BitmapFont* GetFontByName( const HashString& name );

		template< class WidgetSubclass >
		void RegisterFactory( const HashString& type );
		AbstractWidgetFactory* GetFactory( const HashString& type ) const;

		template< class WidgetSubclass = Widget >
		WidgetSubclass* CreateWidgetByType( const HashString& type, const HashString& name );
		template< class WidgetSubclass = Widget >
		WidgetSubclass* CreateWidgetFromTemplate( const HashString& templateName );
		template< class WidgetSubclass = Widget >
		WidgetSubclass* CreateWidgetFromTemplate( const HashString& templateName, const HashString& name );
		template< class WidgetSubclass = Widget >
		WidgetSubclass* CreateWidgetFromTemplate( const WidgetTemplate& widgetTemplate, const HashString& name = "" );

		template< class WidgetSubclass = Widget >
		WidgetSubclass* FindWidgetUnderPointer( float x, float y ) const;

		void DestroyWidget( Widget* widget );

		void Update( float elapsedTime );
		void Draw( const Camera& camera );
		bool PointerDown( float x, float y, size_t which );
		bool PointerUp( float x, float y, size_t which );
		bool PointerMotion( float x, float y, float dx, float dy, size_t which );

		void LoadTheme( const char* file );
		WidgetTemplate* CreateTemplate( const HashString& name );
		WidgetTemplate* GetTemplate( const HashString& name ) const;
		WidgetTemplate* LoadTemplateFromFile( const HashString& name, const std::string& file );
		WidgetTemplate* LoadTemplateFromXML( const HashString& name, const XmlReader::XmlReaderIterator& xml );
		void DestroyTemplate( const HashString& name );
		void DestroyAllTemplates();

		Widget* GetRootWidget() const;

		bool IsInitialized() const;

	private:
		void BuildWidgetTemplateFromXML( const XmlReader::XmlReaderIterator& xml, WidgetTemplate& widgetTemplate );

		bool mIsInitialized;
		Widget* mRootWidget;
		HashMap< WidgetTemplate* > mTemplatesByName;
		HashMap< AbstractWidgetFactory* > mFactoriesByType;
		HashMap< BitmapFont* > mFonts;
	};
	//---------------------------------------
	template< typename WidgetSubclass >
	void WidgetManager::RegisterFactory( const HashString& type )
	{
		assertion( IsInitialized(), "Cannot register factory for WidgetManager that is not initialized!" );
		assertion( !type.GetString().empty(), "Cannot register factory with empty class name!" );

		// Make sure there isn't already a factory with the passed name.
		assertion( GetFactory( type ) == nullptr, "A Widget factory with the class name \"%s\" already exists!", type.GetCString() );

		// Create a new factory that services Widgets of the specified type.
		mFactoriesByType[ type ] = new WidgetFactory< WidgetSubclass >();
		DebugPrintf( "Registered Widget factory: \"%s\" = %s", type.GetCString(), WidgetSubclass::TYPE.GetName() );
	}
	//---------------------------------------
	template< typename WidgetSubclass >
	WidgetSubclass* WidgetManager::CreateWidgetByType( const HashString& type, const HashString& name )
	{
		assertion( IsInitialized(), "Cannot create Widget for WidgetManager that is not initialized!" );

		Widget* base = nullptr;
		WidgetSubclass* derived = nullptr;

		// Look up the factory by its class name.
		AbstractWidgetFactory* factory = GetFactory( type );

		if( factory )
		{
			// If a factory was found, create a new Widget instance.
			base = factory->CreateWidget( this, name );

			// Cast the new Widget to the derived class.
			derived = dynamic_cast< WidgetSubclass* >( base );

			if( !derived )
			{
				// If the Widget could not be cast to the proper class type, return nullptr and post a warning.
				WarnFail( "Could not load Widget \"%s\" because the Widget type \"%s\" could not be cast to the required type (\"%s\")!\n", name.GetCString(), base->GetType().GetName(), WidgetSubclass::TYPE.GetName() );

				// Destroy the Widget.
				delete base;
			}
		}
		else
		{
			// If no factory was found, return nullptr and post a warning.
			WarnFail( "Cannot instantiate unknown widget type \"%s\"\n", type.GetCString() );
		}

		// Return the new Widget.
		return derived;
	}
	//---------------------------------------
	template< class WidgetSubclass >
	WidgetSubclass* WidgetManager::CreateWidgetFromTemplate( const HashString& templateName )
	{
		return WidgetManager::CreateWidgetFromTemplate( templateName, HashString() );
	}
	//---------------------------------------
	template< class WidgetSubclass >
	WidgetSubclass* WidgetManager::CreateWidgetFromTemplate( const HashString& templateName, const HashString& name )
	{
		assertion( IsInitialized(), "Cannot create Widget from template for WidgetManager that is not initialized!" );

		WidgetSubclass* widget = nullptr;

		// Look up the template.
		const WidgetTemplate* widgetTemplate = GetTemplate( templateName );

		if( widgetTemplate != nullptr )
		{
			DebugPrintf( "Resolving includes for template \"%s\"...", templateName.GetCString() );

			// Copy the template and resolve all includes.
			WidgetTemplate resolvedTemplate( *widgetTemplate );
			resolvedTemplate.ResolveIncludes( this );

			// Create the Widget using the resolved template.
			widget = CreateWidgetFromTemplate< WidgetSubclass >( resolvedTemplate, name );
		}
		else
		{
			WarnFail( "Could not create Widget from template because the template \"%s\" was not found!", name.GetCString() );
		}

		return widget;
	}
	//---------------------------------------
	template< class WidgetSubclass >
	WidgetSubclass* WidgetManager::CreateWidgetFromTemplate( const WidgetTemplate& widgetTemplate, const HashString& name )
	{
		assertion( IsInitialized(), "Cannot create Widget from template for WidgetManager that is not initialized!" );

		WidgetSubclass* widget = nullptr;

		// Use the name provided by the template (if possible).
		HashString widgetName = name;

		if( widgetName.GetString().empty() && widgetTemplate.HasProperty( WidgetTemplate::PROPERTY_NAME ) )
		{
			// If no widget name was specified, use the one from the template (if any).
			widgetName = widgetTemplate.GetProperty( WidgetTemplate::PROPERTY_NAME );
		}

		if( widgetTemplate.HasProperty( WidgetTemplate::PROPERTY_TYPE ) )
		{
			// Get the type and name of the Widget to create.
			HashString type = widgetTemplate.GetProperty( WidgetTemplate::PROPERTY_TYPE );

			if( !widgetName.GetString().empty() )
			{
				// Create a new Widget with the specified class and name and load it from XML.
				widget = CreateWidgetByType< WidgetSubclass >( type, widgetName );

				if( widget )
				{
					// Let the Widget load itself from properties.
					widget->LoadFromTemplate( widgetTemplate );
					widget->Init();

					// Copy the list of child properties.
					const WidgetTemplate::TemplateList children = widgetTemplate.GetChildren();

					for( auto it = children.begin(); it != children.end(); ++it )
					{
						// Load all children.
						Widget* child = CreateWidgetFromTemplate( it->second );

						if( child )
						{
							// If the child was loaded successfully, add it to its parent.
							widget->AddChild( child );
						}
					}
				}
			}
			else
			{
				WarnFail( "Cannot create Widget from template because no \"%s\" property was specified!", WidgetTemplate::PROPERTY_NAME.GetCString() );
			}
		}
		else
		{
			WarnFail( "Cannot create Widget from template because no \"%s\" property was found!", WidgetTemplate::PROPERTY_TYPE.GetCString() );
		}

		return widget;
	}
	//---------------------------------------
	template< class WidgetSubclass >
	WidgetSubclass* WidgetManager::FindWidgetUnderPointer( float x, float y ) const
	{
		Widget* result = nullptr;

		// Find all children underneath the pointer (in draw order).
		std::vector< WidgetSubclass* > widgetsUnderPointer;
		mRootWidget->FindDescendantsAt< WidgetSubclass >( x, y, widgetsUnderPointer );

		for( auto it = widgetsUnderPointer.rbegin(); it != widgetsUnderPointer.rend(); ++it )
		{
			Widget* widget = ( *it );

			if( widget->IsVisible() )
			{
				// Find the topmost visible widget under the mouse cursor.
				result = widget;
				break;
			}
		}

		return result;
	}
	//---------------------------------------
	inline bool WidgetManager::IsInitialized() const
	{
		return mIsInitialized;
	}
	//---------------------------------------
	inline Widget* WidgetManager::GetRootWidget() const
	{
		return mRootWidget;
	}
}
