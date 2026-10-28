#ifndef METEORSHIP_DAMAGE_H
#define METEORSHIP_DAMAGE_H

namespace ms::components {
    enum class DamageType {
        Projectile,
        Fire,
        Frost,
        Electric,
        Poison,
        Explosion
    };

    struct DamageAmount {
        DamageType type = DamageType::Projectile;
        float amount = 1.f;
    };
}

#endif //METEORSHIP_DAMAGE_H
