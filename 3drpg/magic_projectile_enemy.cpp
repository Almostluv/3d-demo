/**
 * @file
 * @brief magic_projectile_enemy.cpp 実装を定義する。
 */

#include "magic_projectile_enemy.h"
#include "audio_manager.h"
#include "debug_draw.h"
#include "enemy_attack_range_marker.h"
#include "model_resource.h"
#include "player_param.h"
#include "resource_manager.h"
#include "sound_resource.h"
#include "DxLib.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr const char* DarkMagicBallGraph = "Resource/Effect/DarkMagicBall.png"; ///< 魔法弾の加算描画に使う画像。
constexpr float ShotInterval = 3.0f; ///< 通常の誘導弾シーケンス開始間隔。単位は seconds。
constexpr float ShotSequenceInterval = 0.45f; ///< 連続誘導弾を 1 発ずつ生成する間隔。単位は seconds。
constexpr float ShotLaunchDelay = 1.0f; ///< 生成された誘導弾が発射されるまでの待機時間。単位は seconds。
constexpr float PincerInterval = 5.0f; ///< HP 40% 以下で使う挟み込み攻撃の間隔。単位は seconds。
constexpr float BurstInterval = 6.5f; ///< HP 40% 以下で使う四方向攻撃の間隔。単位は seconds。
constexpr float ProjectileSpeed = 42.0f; ///< 魔法弾の基本移動速度。ワールド空間。
constexpr float ProjectileLife = 4.0f; ///< 魔法弾が自動消滅するまでの寿命。単位は seconds。
constexpr float ProjectileRotationSpeed = -10.0f; ///< 魔法弾ビルボードの回転速度。
constexpr float ProjectileHitRadius = 4.5f; ///< 魔法弾の球状当たり判定半径。
constexpr int ProjectileDamage = 3; ///< 魔法弾がプレイヤーに与えるダメージ。
constexpr int VampireHp = 30; ///< ボスの最大 HP。
constexpr float VampireScale = 0.16f; ///< MV1 モデルへ適用する描画スケール。
constexpr float VampireHitRadius = 18.0f; ///< ボス本体のカプセル当たり判定半径。
constexpr float VampireHitHeight = 34.0f; ///< ボス本体のカプセル当たり判定高さ。
constexpr float VampireHeadY = 13.0f; ///< 本体位置から頭部基準点までの Y オフセット。
constexpr float IntroProjectileHoverY = VampireHeadY + 34.0f; ///< イントロ魔法弾が待機する高さ。
constexpr float IntroProjectileRiseSpeed = 24.0f; ///< イントロ魔法弾が待機高度へ上がる速度。
constexpr float IntroProjectileHomingTime = 1.05f; ///< 発射直後に対象へ向きを補正する時間。単位は seconds。
constexpr float IntroProjectileTurnRate = 1.85f; ///< 誘導中に現在方向から目標方向へ寄せる強さ。
constexpr float ProjectileArcTime = 1.0f; ///< 発射直後に上向き補正をかける時間。単位は seconds。
constexpr float ProjectileArcLift = 38.0f; ///< 弧を描かせるために加える Y 方向速度。
constexpr int ProjectileTrailMax = 12; ///< 軌跡として保持する過去位置の最大数。
constexpr float ProjectileImpactDuration = 0.32f; ///< 着弾エフェクトの表示時間。単位は seconds。
constexpr float StaggerTime = 0.7f; ///< 被弾フェーズ遷移時の怯み時間。単位は seconds。
constexpr float CenterTeleportMarkerDuration = 1.1f; ///< 中央テレポート予告の表示時間。単位は seconds。
constexpr float CenterTeleportMarkerRadius = 24.0f; ///< 中央テレポート予告の範囲半径。
constexpr int Phase80Hp = 24; ///< 最大 HP 30 に対する 80% フェーズ境界。
constexpr int Phase60Hp = 18; ///< 最大 HP 30 に対する 60% フェーズ境界。
constexpr int Phase40Hp = 12; ///< 最大 HP 30 に対する 40% フェーズ境界。
constexpr int Phase20Hp = 6; ///< 最大 HP 30 に対する 20% フェーズ境界。
const VECTOR CenterTeleportPos = { 0.0f, 0.0f, -24.0f }; ///< 強攻撃前に移動する中央位置。
/// HP フェーズごとに順番に使う側面テレポート位置。
const VECTOR SideTeleportPositions[3] =
{
    { -96.0f, 0.0f, -24.0f },
    { 96.0f, 0.0f, -24.0f },
    { 0.0f, 0.0f, 72.0f }
};
}

MagicProjectileEnemy::MagicProjectileEnemy(VECTOR pos)
    : mModelHandle(ResourceManager::instance()->createModelInstance(ModelResource::Vampire::Model))
    , mIdleAnimSourceHandle(ResourceManager::instance()->getModel(ModelResource::Vampire::Idle))
    , mRoaringAnimSourceHandle(ResourceManager::instance()->getModel(ModelResource::Vampire::Roaring))
    , mHitAnimSourceHandle(ResourceManager::instance()->getModel(ModelResource::Vampire::Hit))
    , mDyingAnimSourceHandle(ResourceManager::instance()->getModel(ModelResource::Vampire::Dying))
    , mSpineFrameIndex(-1)
    , mPos(pos)
    , mHp(VampireHp)
    , mDyingStarted(false)
    , mProjectileGraph(ResourceManager::instance()->getGraph(DarkMagicBallGraph))
    , mShotTimer(0.6f)
    , mShotSequenceTimer(0.0f)
    , mShotSequenceIndex(2)
    , mPincerTimer(3.0f)
    , mBurstTimer(4.5f)
    , mStaggerTimer(0.0f)
    , mCenterTeleportMarkerTimer(0.0f)
    , mCenterTeleportMarkerElapsed(0.0f)
    , mHasPendingTeleport(false)
    , mPendingFourWayBurst(false)
    , mPendingPincer(false)
    , mWaitingCenterTeleport(false)
    , mPendingTeleportPos(pos)
    , mTeleportIndex(0)
    , mPhase80Triggered(false)
    , mPhase60Triggered(false)
    , mPhase40Triggered(false)
    , mPhase20Triggered(false)
{
    if (mModelHandle != -1)
    {
        MV1SetScale(mModelHandle, VGet(VampireScale, VampireScale, VampireScale));
        MV1SetPosition(mModelHandle, mPos);
        MV1SetRotationXYZ(mModelHandle, VGet(0.0f, DX_PI_F, 0.0f));
        mSpineFrameIndex = MV1SearchFrame(mModelHandle, "mixamorig:Spine");
        mAnimator.init(mModelHandle);
        /// 一部のアセットがない環境でも動かせるよう、存在しないアニメーションは無効扱いにする。
        if (mDyingAnimSourceHandle == -1 || MV1GetAnimNum(mDyingAnimSourceHandle) <= 0)
        {
            mDyingAnimSourceHandle = -1;
        }
        if (mHitAnimSourceHandle == -1 || MV1GetAnimNum(mHitAnimSourceHandle) <= 0)
        {
            mHitAnimSourceHandle = -1;
        }
        if (mIdleAnimSourceHandle != -1 && MV1GetAnimNum(mIdleAnimSourceHandle) > 0)
        {
            mAnimator.play(0, mIdleAnimSourceHandle, true, 0.0f);
        }
    }
}

void MagicProjectileEnemy::update(float deltaTime, Player* player, Player* companion)
{
    updateAnimation(deltaTime);
    /// 着弾エフェクトはボスの生死に関係なく短時間だけ残す。
    for (auto& impact : mImpacts)
    {
        impact.timer += deltaTime;
    }
    mImpacts.erase(
        std::remove_if(
            mImpacts.begin(),
            mImpacts.end(),
            [](const MagicImpact& impact)
            {
                return impact.timer >= ProjectileImpactDuration;
            }),
        mImpacts.end());

    if (isDead())
    {
        /// 死亡後も画面上に残った弾だけは寿命や衝突を更新する。
        updateProjectiles(deltaTime, player, companion);
        return;
    }

    if (mWaitingCenterTeleport)
    {
        /// テレポート予告中は通常攻撃を止め、着地点の警告表示を優先する。
        updateCenterTeleportMarker(deltaTime, player, companion);
        updateProjectiles(deltaTime, player, companion);
        return;
    }

    if (mStaggerTimer > 0.0f)
    {
        /// 怯み中は次のフェーズ攻撃へ入る準備だけを進める。
        updateStagger(deltaTime, player, companion);
        updateProjectiles(deltaTime, player, companion);
        return;
    }

    Player* target = chooseTarget(player, companion);

    if (mShotSequenceIndex < 2)
    {
        /// 2 発を同時に出さず、短い間隔でずらして発射予告を作る。
        mShotSequenceTimer -= deltaTime;
        if (mShotSequenceTimer <= 0.0f)
        {
            spawnWaitingHomingShot(mShotSequenceIndex);
            ++mShotSequenceIndex;
            mShotSequenceTimer = ShotSequenceInterval;
            if (mShotSequenceIndex >= 2)
            {
                mShotTimer = ShotInterval;
            }
        }
    }
    else
    {
        mShotTimer -= deltaTime;
        if (mShotTimer <= 0.0f)
        {
            startHomingShotSequence();
        }
    }

    if (mHp <= Phase40Hp)
    {
        /// 40% 以下では通常誘導弾に加えて、定期的な範囲攻撃を重ねる。
        mPincerTimer -= deltaTime;
        if (mPincerTimer <= 0.0f)
        {
            if (target != nullptr && !target->isDead())
            {
                shootPincer(target->getPosition());
            }
            mPincerTimer = PincerInterval;
        }

        mBurstTimer -= deltaTime;
        if (mBurstTimer <= 0.0f)
        {
            shootFourWayBurst();
            mBurstTimer = BurstInterval;
        }
    }

    updateProjectiles(deltaTime, player, companion);
}

void MagicProjectileEnemy::updateAnimation(float deltaTime)
{
    mAnimator.update(deltaTime);
    for (auto& projectile : mProjectiles)
    {
        projectile.rotation += ProjectileRotationSpeed * deltaTime;
    }
}

void MagicProjectileEnemy::draw() const
{
    if (mWaitingCenterTeleport)
    {
        VECTOR markerCenter = VAdd(CenterTeleportPos, VGet(0.0f, 0.25f, 0.0f));
        bool landingSoon = mCenterTeleportMarkerTimer <= 0.35f;
        drawEnemyAttackRangeMarker(markerCenter, CenterTeleportMarkerRadius, landingSoon, mCenterTeleportMarkerElapsed);
    }

    if (!isDead() || mDyingStarted)
    {
        if (mModelHandle != -1)
        {
            MV1DrawModel(mModelHandle);
        }
    }

    SetUseLighting(FALSE);
    SetUseZBuffer3D(TRUE);
    SetWriteZBuffer3D(FALSE);
    SetUseBackCulling(FALSE);
    for (const auto& projectile : mProjectiles)
    {
        /// 軌跡は古い点ほど薄く、現在位置に近い点ほど強く描画する。
        for (int i = 1; i < static_cast<int>(projectile.trail.size()); ++i)
        {
            VECTOR fromPos = projectile.trail[i - 1];
            VECTOR toPos = projectile.trail[i];
            if (i == static_cast<int>(projectile.trail.size()) - 1)
            {
                VECTOR away = VSub(fromPos, projectile.pos);
                if (VSize(away) > 0.0001f)
                {
                    toPos = VAdd(projectile.pos, VScale(VNorm(away), 7.5f));
                }
            }

            if (VSize(VSub(toPos, fromPos)) <= 0.0001f)
            {
                continue;
            }

            float rate = static_cast<float>(i) / static_cast<float>(projectile.trail.size());
            int alpha = static_cast<int>(150.0f * rate);
            int color = GetColor(115, 40, 220);
            SetDrawBlendMode(DX_BLENDMODE_ADD, alpha);
            DrawLine3D(fromPos, toPos, color);
            DrawSphere3D(toPos, 1.2f + 2.6f * rate, 8, color, GetColor(215, 170, 255), TRUE);
        }

        if (mProjectileGraph != -1)
        {
            SetDrawBlendMode(DX_BLENDMODE_ADD, 235);
            SetDrawBright(190, 120, 255);
            DrawBillboard3D(projectile.pos, 0.5f, 0.5f, 15.0f, projectile.rotation, mProjectileGraph, TRUE);
        }
        else
        {
            DrawSphere3D(projectile.pos, 4.8f, 12, GetColor(80, 20, 160), GetColor(220, 180, 255), TRUE);
        }
    }

    for (const auto& impact : mImpacts)
    {
        /// rate を 0..1 に収めて、エフェクトサイズと透明度を同じ進行率で制御する。
        float rate = impact.timer / ProjectileImpactDuration;
        if (rate > 1.0f)
        {
            rate = 1.0f;
        }
        float fade = 1.0f - rate;
        int alpha = static_cast<int>(230.0f * fade);
        VECTOR pos = VAdd(impact.pos, VGet(0.0f, 5.0f, 0.0f));
        int dark = GetColor(45, 0, 80);
        int purple = GetColor(170, 70, 255);
        SetDrawBlendMode(DX_BLENDMODE_ADD, alpha);
        DrawSphere3D(pos, 5.0f + rate * 7.0f, 12, purple, dark, FALSE);
        for (int i = 0; i < 10; ++i)
        {
            float angle = DX_TWO_PI_F * static_cast<float>(i) / 10.0f;
            VECTOR dir = VGet(cosf(angle), 0.25f + sinf(angle * 2.0f) * 0.15f, sinf(angle));
            DrawLine3D(pos, VAdd(pos, VScale(dir, 8.0f + rate * 18.0f)), purple);
        }
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    SetDrawBright(255, 255, 255);
    SetUseBackCulling(TRUE);
    SetWriteZBuffer3D(TRUE);
    SetUseLighting(TRUE);
}

void MagicProjectileEnemy::drawDebugHitCollision() const
{
    unsigned int projectileHitColor = GetColor(190, 80, 255);
    for (const auto& projectile : mProjectiles)
    {
        drawDebugCircleXZ(projectile.pos, ProjectileHitRadius, projectileHitColor, 0.0f);
    }

    drawDebugCapsuleY(mPos, VampireHitRadius, VampireHitHeight, GetColor(255, 255, 255));
}

bool MagicProjectileEnemy::isDead() const
{
    return mHp <= 0;
}

bool MagicProjectileEnemy::isDeadAnimationFinished() const
{
    return isDead() && (mDyingAnimSourceHandle != -1 ? mAnimator.isFinished() : true);
}

VECTOR MagicProjectileEnemy::getPosition() const
{
    return mPos;
}

VECTOR MagicProjectileEnemy::getHeadPosition() const
{
    return VAdd(mPos, VGet(0.0f, VampireHeadY, 0.0f));
}

VECTOR MagicProjectileEnemy::getLockPointPosition() const
{
    if (mModelHandle != -1 && mSpineFrameIndex != -1)
    {
        return MV1GetFramePosition(mModelHandle, mSpineFrameIndex);
    }

    return getHeadPosition();
}

int MagicProjectileEnemy::getHp() const
{
    return mHp;
}

int MagicProjectileEnemy::getMaxHp() const
{
    return VampireHp;
}

int MagicProjectileEnemy::getProjectileCount() const
{
    return static_cast<int>(mProjectiles.size());
}

float MagicProjectileEnemy::getNearestMovingProjectileDistance(VECTOR pos) const
{
    float nearest = 1000000.0f;
    for (const auto& projectile : mProjectiles)
    {
        if (VSize(projectile.velocity) <= 0.0001f)
        {
            continue;
        }

        VECTOR diff = VSub(projectile.pos, pos);
        diff.y = 0.0f;
        float distance = VSize(diff);
        if (distance < nearest)
        {
            nearest = distance;
        }
    }
    return nearest;
}

bool MagicProjectileEnemy::getNearestMovingProjectileInfo(VECTOR pos, VECTOR* projectilePos, VECTOR* velocity) const
{
    const MagicProjectile* nearestProjectile = nullptr;
    float nearest = 1000000.0f;
    for (const auto& projectile : mProjectiles)
    {
        if (VSize(projectile.velocity) <= 0.0001f)
        {
            continue;
        }

        VECTOR diff = VSub(projectile.pos, pos);
        diff.y = 0.0f;
        float distance = VSize(diff);
        if (distance < nearest)
        {
            nearest = distance;
            nearestProjectile = &projectile;
        }
    }

    if (nearestProjectile == nullptr)
    {
        return false;
    }

    if (projectilePos != nullptr)
    {
        *projectilePos = nearestProjectile->pos;
    }
    if (velocity != nullptr)
    {
        *velocity = nearestProjectile->velocity;
    }
    return true;
}

float MagicProjectileEnemy::getHitRadius() const
{
    return VampireHitRadius;
}

float MagicProjectileEnemy::getHitHeight() const
{
    return VampireHitHeight;
}

bool MagicProjectileEnemy::isInsideAttackCapsule(VECTOR attackerPos, VECTOR forward, float attackRadius) const
{
    forward.y = 0.0f;
    if (VSize(forward) <= 0.0001f)
    {
        return false;
    }
    forward = VNorm(forward);

    VECTOR toTarget = VSub(mPos, attackerPos);
    toTarget.y = 0.0f;
    float distance = VSize(toTarget);
    if (distance <= 0.0001f)
    {
        return true;
    }

    VECTOR attackDir = VNorm(toTarget);
    float attackDot = forward.x * attackDir.x + forward.z * attackDir.z;
    if (attackDot < PlayerParam::AttackFrontDotThreshold)
    {
        /// プレイヤーの前方判定外にいる場合、距離が近くても近接攻撃は当てない。
        return false;
    }

    return distance <= attackRadius + VampireHitRadius;
}

void MagicProjectileEnemy::damage(int value)
{
    if (isDead())
    {
        return;
    }

    int oldHp = mHp;
    mHp -= value;
    if (mHp < 0)
    {
        mHp = 0;
    }

    if (mHp <= 0)
    {
        /// 死亡時は保留中のフェーズ処理を破棄し、残弾も消して演出を死亡状態へ一本化する。
        mHasPendingTeleport = false;
        mPendingFourWayBurst = false;
        mPendingPincer = false;
        mWaitingCenterTeleport = false;
        mStaggerTimer = 0.0f;
        mDyingStarted = true;
        mProjectiles.clear();
        if (mDyingAnimSourceHandle != -1 && MV1GetAnimNum(mDyingAnimSourceHandle) > 0)
        {
            mAnimator.play(0, mDyingAnimSourceHandle, false, 0.12f);
        }
        return;
    }

    checkPhaseThreshold(oldHp);
}

void MagicProjectileEnemy::debugSetHpToOne()
{
    if (!isDead() && mHp > 1)
    {
        mHp = 1;
    }
}

void MagicProjectileEnemy::playIdle()
{
    if (mIdleAnimSourceHandle != -1 && MV1GetAnimNum(mIdleAnimSourceHandle) > 0)
    {
        mAnimator.play(0, mIdleAnimSourceHandle, true, 0.12f);
    }
}

void MagicProjectileEnemy::startRoaring()
{
    AudioManager::instance()->playSe3D(SoundResource::Se::Enemy::VampireOpening, getHeadPosition());

    if (mRoaringAnimSourceHandle != -1 && MV1GetAnimNum(mRoaringAnimSourceHandle) > 0)
    {
        mAnimator.play(0, mRoaringAnimSourceHandle, false, 0.12f);
    }
}

bool MagicProjectileEnemy::isRoaringFinished() const
{
    return mAnimator.isFinished();
}

void MagicProjectileEnemy::updateIntroProjectiles(float deltaTime)
{
    float hoverY = mPos.y + IntroProjectileHoverY;
    for (auto& projectile : mProjectiles)
    {
        if (projectile.introHoming || VSize(projectile.velocity) > 0.0001f)
        {
            continue;
        }

        projectile.life = ProjectileLife;
        projectile.rotation += ProjectileRotationSpeed * deltaTime;
        /// 発射前の魔法弾は寿命で消えないよう更新しつつ、頭上の待機高度まで上げる。
        if (projectile.pos.y < hoverY)
        {
            projectile.pos.y += IntroProjectileRiseSpeed * deltaTime;
            if (projectile.pos.y > hoverY)
            {
                projectile.pos.y = hoverY;
            }
        }
    }
}

void MagicProjectileEnemy::setIntroMagicBallCount(int count)
{
    /// イントロ演出は最大 4 発までを前提に配置オフセットを決めている。
    if (count < 0) count = 0;
    if (count > 4) count = 4;

    while (static_cast<int>(mProjectiles.size()) < count)
    {
        int index = static_cast<int>(mProjectiles.size());
        float offsetX = -18.0f + static_cast<float>(index) * 12.0f;
        MagicProjectile projectile;
        projectile.pos = VAdd(mPos, VGet(offsetX, VampireHeadY, 18.0f));
        projectile.velocity = VGet(0.0f, 0.0f, 0.0f);
        projectile.life = ProjectileLife;
        projectile.rotation = 0.0f;
        projectile.introHoming = false;
        projectile.homingTimer = 0.0f;
        projectile.waitingLaunch = false;
        projectile.launchTimer = 0.0f;
        projectile.targetIndex = -1;
        projectile.arcTimer = 0.0f;
        projectile.trail.push_back(projectile.pos);
        mProjectiles.push_back(projectile);
    }
}

void MagicProjectileEnemy::releaseIntroMagicBalls()
{

    /// イントロで浮かせた弾を通常の待機発射弾へ切り替える。
    for (int i = 0; i < static_cast<int>(mProjectiles.size()) && i < 4; ++i)
    {
        mProjectiles[i].velocity = VGet(0.0f, 0.0f, 0.0f);
        mProjectiles[i].life = ProjectileLife;
        mProjectiles[i].introHoming = false;
        mProjectiles[i].homingTimer = 0.0f;
        mProjectiles[i].waitingLaunch = true;
        mProjectiles[i].launchTimer = ShotLaunchDelay;
        mProjectiles[i].targetIndex = i % 2;
        mProjectiles[i].arcTimer = 0.0f;
    }
}

Player* MagicProjectileEnemy::chooseTarget(Player* player, Player* companion) const
{
    Player* target = player;
    if (target != nullptr && target->isDead())
    {
        target = nullptr;
    }

    if (companion != nullptr && !companion->isDead())
    {
        if (target == nullptr)
        {
            return companion;
        }

        /// 生存している候補が複数いる場合、ボスに近い方を狙う。
        VECTOR toTarget = VSub(target->getPosition(), mPos);
        VECTOR toCompanion = VSub(companion->getPosition(), mPos);
        toTarget.y = 0.0f;
        toCompanion.y = 0.0f;
        if (VSize(toCompanion) < VSize(toTarget))
        {
            target = companion;
        }
    }

    return target;
}

void MagicProjectileEnemy::startHomingShotSequence()
{
    mShotSequenceIndex = 0;
    mShotSequenceTimer = 0.0f;
}

void MagicProjectileEnemy::spawnWaitingHomingShot(int targetIndex)
{
    float offsetX = targetIndex == 0 ? -8.0f : 8.0f;
    MagicProjectile projectile;
    projectile.pos = VAdd(mPos, VGet(offsetX, VampireHeadY, 10.0f));
    projectile.velocity = VGet(0.0f, 0.0f, 0.0f);
    projectile.life = ProjectileLife;
    projectile.rotation = 0.0f;
    projectile.introHoming = false;
    projectile.homingTimer = 0.0f;
    projectile.waitingLaunch = true;
    projectile.launchTimer = ShotLaunchDelay;
    projectile.targetIndex = targetIndex;
    projectile.arcTimer = 0.0f;
    projectile.trail.push_back(projectile.pos);
    mProjectiles.push_back(projectile);
}

void MagicProjectileEnemy::shootFourWayBurst()
{
    VECTOR start = getHeadPosition();
    const VECTOR dirs[4] =
    {
        VGet(1.0f, 0.0f, 0.0f),
        VGet(0.0f, 0.0f, 1.0f),
        VGet(-1.0f, 0.0f, 0.0f),
        VGet(0.0f, 0.0f, -1.0f)
    };

    for (int i = 0; i < 4; ++i)
    {
        spawnProjectile(start, VScale(dirs[i], ProjectileSpeed));
    }
}

void MagicProjectileEnemy::shootPincer(VECTOR targetPos)
{
    VECTOR dir = VSub(targetPos, mPos);
    dir.y = 0.0f;
    if (VSize(dir) <= 0.0001f)
    {
        dir = VGet(0.0f, 0.0f, 1.0f);
    }
    dir = VNorm(dir);

    /// 対象方向に対する左右ベクトルを使い、2 発を横から挟むように撃つ。
    VECTOR side = VGet(-dir.z, 0.0f, dir.x);
    VECTOR head = getHeadPosition();
    spawnProjectile(VAdd(head, VScale(side, 18.0f)), VScale(VNorm(VSub(targetPos, VAdd(mPos, VScale(side, 18.0f)))), ProjectileSpeed));
    spawnProjectile(VSub(head, VScale(side, 18.0f)), VScale(VNorm(VSub(targetPos, VSub(mPos, VScale(side, 18.0f)))), ProjectileSpeed));
}

void MagicProjectileEnemy::spawnProjectile(VECTOR pos, VECTOR velocity)
{
    MagicProjectile projectile;
    projectile.pos = pos;
    projectile.velocity = velocity;
    projectile.life = ProjectileLife;
    projectile.rotation = 0.0f;
    projectile.introHoming = false;
    projectile.homingTimer = 0.0f;
    projectile.waitingLaunch = false;
    projectile.launchTimer = 0.0f;
    projectile.targetIndex = -1;
    projectile.arcTimer = 0.0f;
    projectile.trail.push_back(projectile.pos);
    mProjectiles.push_back(projectile);
}

void MagicProjectileEnemy::updateStagger(float deltaTime, Player* player, Player* companion)
{
    mStaggerTimer -= deltaTime;
    if (mStaggerTimer > 0.0f)
    {
        return;
    }

    if (mPendingFourWayBurst && mHasPendingTeleport)
    {
        /// 強いフェーズ攻撃だけは即テレポートせず、着地点を事前表示する。
        mWaitingCenterTeleport = true;
        mCenterTeleportMarkerTimer = CenterTeleportMarkerDuration;
        mCenterTeleportMarkerElapsed = 0.0f;
        return;
    }

    finishPendingTeleport(player, companion);
}

void MagicProjectileEnemy::updateCenterTeleportMarker(float deltaTime, Player* player, Player* companion)
{
    mCenterTeleportMarkerTimer -= deltaTime;
    mCenterTeleportMarkerElapsed += deltaTime;
    if (mCenterTeleportMarkerTimer > 0.0f)
    {
        return;
    }

    mWaitingCenterTeleport = false;
    finishPendingTeleport(player, companion);
}

void MagicProjectileEnemy::finishPendingTeleport(Player* player, Player* companion)
{
    if (mHasPendingTeleport)
    {
        teleportTo(mPendingTeleportPos);
        mHasPendingTeleport = false;
    }
    playIdle();

    if (mPendingFourWayBurst || mPendingPincer)
    {
        /// フェーズ遷移直後はまず誘導弾シーケンスで攻撃再開を見せる。
        startHomingShotSequence();
    }
    mPendingFourWayBurst = false;
    mPendingPincer = false;
}

void MagicProjectileEnemy::startHitStagger(VECTOR teleportPos, bool fourWayBurst, bool pincer)
{
    mStaggerTimer = StaggerTime;
    mHasPendingTeleport = true;
    mPendingTeleportPos = teleportPos;
    mPendingFourWayBurst = fourWayBurst;
    mPendingPincer = pincer;
    mShotTimer = ShotInterval;
    mPincerTimer = PincerInterval;
    mBurstTimer = BurstInterval;
    if (mHitAnimSourceHandle != -1 && MV1GetAnimNum(mHitAnimSourceHandle) > 0)
    {
        mAnimator.play(0, mHitAnimSourceHandle, false, 0.08f);
    }
}

void MagicProjectileEnemy::teleportTo(VECTOR pos)
{
    mPos = pos;
    if (mModelHandle != -1)
    {
        MV1SetPosition(mModelHandle, mPos);
        VECTOR toCenter = VSub(CenterTeleportPos, mPos);
        toCenter.y = 0.0f;
        if (VSize(toCenter) > 0.0001f)
        {
            /// テレポート後は中央側を向かせ、次の攻撃の見た目を揃える。
            float yaw = std::atan2(toCenter.x, toCenter.z) + DX_PI_F;
            MV1SetRotationXYZ(mModelHandle, VGet(0.0f, yaw, 0.0f));
        }
    }
}

VECTOR MagicProjectileEnemy::nextSideTeleportPosition()
{
    VECTOR pos = SideTeleportPositions[mTeleportIndex % 3];
    ++mTeleportIndex;
    return pos;
}

void MagicProjectileEnemy::checkPhaseThreshold(int oldHp)
{
    /// HP が境界をまたいだ瞬間だけフェーズ処理を開始する。
    if (!mPhase40Triggered && oldHp > Phase40Hp && mHp <= Phase40Hp)
    {
        mPhase40Triggered = true;
        startHitStagger(CenterTeleportPos, true, true);
        return;
    }
    if (!mPhase80Triggered && oldHp > Phase80Hp && mHp <= Phase80Hp)
    {
        mPhase80Triggered = true;
        startHitStagger(nextSideTeleportPosition(), false, false);
        return;
    }
    if (!mPhase60Triggered && oldHp > Phase60Hp && mHp <= Phase60Hp)
    {
        mPhase60Triggered = true;
        startHitStagger(nextSideTeleportPosition(), false, false);
        return;
    }
    if (!mPhase20Triggered && oldHp > Phase20Hp && mHp <= Phase20Hp)
    {
        mPhase20Triggered = true;
        startHitStagger(nextSideTeleportPosition(), false, false);
    }
}

void MagicProjectileEnemy::updateProjectiles(float deltaTime, Player* player, Player* companion)
{
    for (auto& projectile : mProjectiles)
    {
        projectile.life -= deltaTime;
        projectile.rotation += ProjectileRotationSpeed * deltaTime;

        if (projectile.waitingLaunch)
        {
            float hoverY = mPos.y + IntroProjectileHoverY;
            projectile.life = ProjectileLife;
            /// 発射待ちの弾は寿命を固定し、頭上に到達してからカウントダウンを始める。
            if (projectile.pos.y < hoverY)
            {
                projectile.pos.y += IntroProjectileRiseSpeed * deltaTime;
                if (projectile.pos.y > hoverY)
                {
                    projectile.pos.y = hoverY;
                }
            }
            else
            {
                projectile.launchTimer -= deltaTime;
            }

            if (projectile.launchTimer <= 0.0f)
            {
                Player* target = projectile.targetIndex == 0 ? player : projectile.targetIndex == 1 ? companion : chooseTarget(player, companion);
                if (target == nullptr || target->isDead())
                {
                    target = chooseTarget(player, companion);
                }
                VECTOR dir = target != nullptr ? VSub(VAdd(target->getPosition(), VGet(0.0f, 6.0f, 0.0f)), projectile.pos) : VGet(0.0f, -0.2f, 1.0f);
                if (VSize(dir) <= 0.0001f)
                {
                    dir = VGet(0.0f, -0.2f, 1.0f);
                }
                /// 初速は少し抑えて、直後の誘導補正で曲がって見える余地を残す。
                projectile.velocity = VScale(VNorm(dir), ProjectileSpeed * 0.85f);
                projectile.introHoming = true;
                projectile.homingTimer = IntroProjectileHomingTime;
                projectile.arcTimer = ProjectileArcTime;
                projectile.waitingLaunch = false;
            }
        }

        if (projectile.introHoming && projectile.homingTimer > 0.0f)
        {
            Player* target = projectile.targetIndex == 0 ? player : projectile.targetIndex == 1 ? companion : chooseTarget(player, companion);
            if (target == nullptr || target->isDead())
            {
                target = chooseTarget(player, companion);
            }
            if (target != nullptr)
            {
                VECTOR targetPos = VAdd(target->getPosition(), VGet(0.0f, 6.0f, 0.0f));
                VECTOR desired = VSub(targetPos, projectile.pos);
                if (VSize(desired) > 0.0001f)
                {
                    desired = VNorm(desired);
                    VECTOR current = projectile.velocity;
                    if (VSize(current) <= 0.0001f)
                    {
                        current = desired;
                    }
                    else
                    {
                        current = VNorm(current);
                    }

                    float turn = IntroProjectileTurnRate * deltaTime;
                    if (turn > 1.0f)
                    {
                        turn = 1.0f;
                    }
                    /// 現在方向と目標方向を補間し、急旋回ではなく追尾している見た目にする。
                    VECTOR nextDir = VNorm(VAdd(VScale(current, 1.0f - turn), VScale(desired, turn)));
                    projectile.velocity = VScale(nextDir, ProjectileSpeed * 0.95f);
                }
            }
            projectile.homingTimer -= deltaTime;
            if (projectile.homingTimer <= 0.0f)
            {
                projectile.introHoming = false;
            }
        }

        if (projectile.arcTimer > 0.0f && VSize(projectile.velocity) > 0.0001f)
        {
            /// 発射直後だけ上向き速度を足し、地面へ直線的に落ちない弾道にする。
            float arcRate = projectile.arcTimer / ProjectileArcTime;
            projectile.velocity.y += ProjectileArcLift * arcRate * deltaTime;
            projectile.arcTimer -= deltaTime;
            if (projectile.arcTimer < 0.0f)
            {
                projectile.arcTimer = 0.0f;
            }
        }

        projectile.pos = VAdd(projectile.pos, VScale(projectile.velocity, deltaTime));
        if (VSize(projectile.velocity) > 0.0001f)
        {
            projectile.trail.push_back(projectile.pos);
            if (projectile.trail.size() > ProjectileTrailMax)
            {
                projectile.trail.erase(projectile.trail.begin());
            }
        }
        if (VSize(projectile.velocity) > 0.0001f && projectile.pos.y <= mPos.y + 1.0f)
        {
            /// 地面付近まで落ちた移動中の弾は、残り寿命を待たずに消す。
            projectile.life = 0.0f;
            continue;
        }
        if (hitPlayer(projectile, player) || hitPlayer(projectile, companion))
        {
            MagicImpact impact;
            impact.pos = projectile.pos;
            impact.timer = 0.0f;
            mImpacts.push_back(impact);
            projectile.life = 0.0f;
        }
    }

    mProjectiles.erase(
        std::remove_if(
            mProjectiles.begin(),
            mProjectiles.end(),
            [](const MagicProjectile& projectile)
            {
                return projectile.life <= 0.0f;
            }),
        mProjectiles.end());
}


bool MagicProjectileEnemy::hitPlayer(MagicProjectile& projectile, Player* player)
{
    if (player == nullptr || player->isDead())
    {
        return false;
    }

    VECTOR playerPos = player->getPosition();
    float radius = player->getCollisionRadius();
    float height = player->getCollisionHeight();
    float bottomY = playerPos.y + radius;
    float topY = playerPos.y + height - radius;
    if (topY < bottomY)
    {
        topY = bottomY;
    }

    /// プレイヤーを縦カプセルとして扱い、魔法弾中心から最も近い点との距離で判定する。
    VECTOR closest = VGet(playerPos.x, projectile.pos.y, playerPos.z);
    if (closest.y < bottomY)
    {
        closest.y = bottomY;
    }
    if (closest.y > topY)
    {
        closest.y = topY;
    }

    VECTOR diff = VSub(projectile.pos, closest);
    float hitRadius = ProjectileHitRadius + radius;
    if (diff.x * diff.x + diff.y * diff.y + diff.z * diff.z > hitRadius * hitRadius)
    {
        return false;
    }

    player->damage(ProjectileDamage, projectile.pos);
    return true;
}
