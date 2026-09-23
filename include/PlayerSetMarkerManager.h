#pragma once

#ifdef MessageBox
#define MessageBox_BAK MessageBox
#undef MessageBox
#endif

namespace LMU
{
	class PlayerSetMarkerManager
	{
		struct MessageBox
		{
			struct Callback : RE::IMessageBoxCallback
			{
				void Run(std::uint8_t a_optionIndex) final;

				void SetData(float a_wndPointX, float a_wndPointY)
				{
					wndPointX = a_wndPointX;
					wndPointY = a_wndPointY;
				}

				void ClearData()
				{
					wndPointX = 0.0F;
					wndPointY = 0.0F;
				}

				float wndPointX = 0.0F;
				float wndPointY = 0.0F;
			};

			MessageBox();

			RE::BSString title;
			RE::BSTArray<RE::BSString> options;
			RE::BSTSmartPointer<Callback> callback = RE::make_smart<Callback>();
		};

	public:
		static PlayerSetMarkerManager* GetSingleton()
		{
			static PlayerSetMarkerManager singleton;
			return &singleton;
		}

		[[nodiscard]] bool CanPlaceMarker() const { return allowPlaceMarker; }
		void AllowPlaceMarker() { allowPlaceMarker = true; }
		void PlaceMarker(RE::LocalMapMenu* a_localMapMenu, float a_wndPointX, float a_wndPointY);

	private:
		MessageBox messageBox;
		bool allowPlaceMarker = true;
	};
}

#ifdef MessageBox_BAK
#define MessageBox MessageBox_BAK
#undef MessageBox_BAK
#endif
