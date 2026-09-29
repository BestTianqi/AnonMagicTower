#pragma once

#include "Entities/Items.h"
#include "Entities/Player.h"

#include <array>
#include <algorithm>

struct ShopOfferCardPresentation {
    int beforeValue = 0;
    int increase = 0;
    int afterValue = 0;
};

struct ClassicShopPresentation {
    int purchaseNumber = 1;
    int price = 0;
    int gold = 0;
    int missingGold = 0;
    bool affordable = false;
    std::array<ShopOfferCardPresentation, 3> cards{};
};

inline ClassicShopPresentation makeClassicShopPresentation(
    const Player& player, const ClassicShopOffer& offer)
{
    ClassicShopPresentation result;
    result.purchaseNumber = player.shopUseCount + 1;
    result.price = offer.price;
    result.gold = player.gold;
    result.missingGold = std::max(0, offer.price - player.gold);
    result.affordable = result.missingGold == 0;
    result.cards = {{
        {player.hp, offer.hp, player.hp + offer.hp},
        {player.atk, offer.atk, player.atk + offer.atk},
        {player.def, offer.def, player.def + offer.def}
    }};
    return result;
}
