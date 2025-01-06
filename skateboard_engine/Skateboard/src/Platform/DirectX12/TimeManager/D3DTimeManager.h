#pragma once
#include "Skateboard/Time/TimeManager.h"

namespace Skateboard
{
	class D3DTimeManager : public TimeManager
	{
	public:
		D3DTimeManager();
		virtual ~D3DTimeManager() override;

		float& ElapsedTime() override;

		void Reset() override;
		void Start() override;
		void Stop() override;
		void Tick() override;

	};

}