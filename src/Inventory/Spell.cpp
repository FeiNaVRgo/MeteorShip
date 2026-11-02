//
// Created by iformat on 10/29/26.
//

#include "Spell.h"
#include "Turret.h"

namespace ms::inventory {
    std::vector<Payload::PayloadedSpells>& Payload::gatherPayloadedSpells() {
        int howFarToMove = 0;
        for (auto& spell : *turret) {
            howFarToMove += getAmountToMoveForward(spell.getProjectileType());
            pushToTree(spell);
            if (howFarToMove == 0) break;
        }

        flattenTree();
        return toShoot;
    }

}