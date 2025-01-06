#pragma once
#include "Skateboard/Renderer/Animation/SkeletalData/SkeletalData.h"
#include "Skateboard/Renderer/Animation/Animation.h"


namespace Skateboard
{
	
	class SkeletonPose;

	class Animator
	{
	public:
		virtual ~Animator() = default;

		static Animator* Create();

		static Animator* Singleton();

		static void UpdateAnimation(SkeletonPose* skeletalPose, Animation* animation, float elapsedTime) { Singleton()->UpdateAnimationImpl(skeletalPose, animation, elapsedTime); }

		static void LinearBlend(SkeletonPose* poseA, SkeletonPose* poseB, SkeletonPose& out, float alpha) { Singleton()->LinearBlendImpl(poseA, poseB, out, alpha); }


	private:

		virtual void UpdateAnimationImpl(SkeletonPose* skeletalPose, Animation* animation, float elapsedTime) = 0;

		virtual void LinearBlendImpl(SkeletonPose* poseA, SkeletonPose* poseB, SkeletonPose& out, float alpha) = 0;

	};

}