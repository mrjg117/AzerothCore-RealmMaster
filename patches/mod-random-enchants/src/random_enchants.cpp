#include "random_enchants.h"
void rollPossibleEnchant(Player* player, Item* item)
{
    // Check global enable option
    if (!sConfigMgr->GetOption<bool>("RandomEnchants.Enable", true))
        return;
    if (!item || !item->IsInWorld())
        return;
    uint32 itemQuality = item->GetTemplate()->Quality;
    uint32 itemClass = item->GetTemplate()->Class;
    /* eliminates enchanting anything that isn't a recognized quality */
    /* eliminates enchanting anything but weapons/armor */
    /* Only blue+ items receive random enchants to avoid overwriting green item affixes */
    if ((itemQuality > ITEM_QUALITY_LEGENDARY || itemQuality < ITEM_QUALITY_RARE) || (itemClass != ITEM_CLASS_WEAPON && itemClass != ITEM_CLASS_ARMOR))
        return;
    int slotRand[5] = { -1, -1, -1, -1, -1 };
    // PROP enchantment slots 7-11; using them avoids conflict with permanent/temporary enchants
    uint32 slotEnch[5] = { 11, 10, 9, 8, 7 };
    // Fetching the configuration values as float
    float enchantChance1 = sConfigMgr->GetOption<float>("RandomEnchants.EnchantChance1", 80.0f);
    float enchantChance2 = sConfigMgr->GetOption<float>("RandomEnchants.EnchantChance2", 62.5f);
    float enchantChance3 = sConfigMgr->GetOption<float>("RandomEnchants.EnchantChance3", 50.0f);
    float enchantChance4 = sConfigMgr->GetOption<float>("RandomEnchants.EnchantChance4", 40.0f);
    float enchantChance5 = sConfigMgr->GetOption<float>("RandomEnchants.EnchantChance5", 30.0f);
    if (rand_chance() < enchantChance1)
        slotRand[0] = getRandEnchantment(item);
    if (slotRand[0] != -1 && rand_chance() < enchantChance2)
        slotRand[1] = getRandEnchantment(item);
    if (slotRand[1] != -1 && rand_chance() < enchantChance3)
        slotRand[2] = getRandEnchantment(item);
    if (slotRand[2] != -1 && rand_chance() < enchantChance4)
        slotRand[3] = getRandEnchantment(item);
    if (slotRand[3] != -1 && rand_chance() < enchantChance5)
        slotRand[4] = getRandEnchantment(item);
    for (int i = 0; i < 5; i++)
    {
        if (slotRand[i] != -1)
        {
            if (sSpellItemEnchantmentStore.LookupEntry(slotRand[i]))
            {   //Make sure enchantment id exists
                player->ApplyEnchantment(item, EnchantmentSlot(slotEnch[i]), false);
                item->SetEnchantment(EnchantmentSlot(slotEnch[i]), slotRand[i], 0, 0);
                player->ApplyEnchantment(item, EnchantmentSlot(slotEnch[i]), true);
            }
        }
    }
    ChatHandler chathandle = ChatHandler(player->GetSession());
    uint8 loc_idx = player->GetSession()->GetSessionDbLocaleIndex();
    const ItemTemplate* temp = item->GetTemplate();
    std::string name = temp->Name1;
    if (ItemLocale const* il = sObjectMgr->GetItemLocale(temp->ItemId))
        ObjectMgr::GetLocaleString(il->Name, loc_idx, name);
    if (slotRand[4] != -1)
        chathandle.PSendSysMessage("Newly Acquired |cffFF0000 {} |rhas received|cffFF0000 5 |rrandom enchantments!", name);
    else if (slotRand[3] != -1)
        chathandle.PSendSysMessage("Newly Acquired |cffFF0000 {} |rhas received|cffFF0000 4 |rrandom enchantments!", name);
    else if (slotRand[2] != -1)
        chathandle.PSendSysMessage("Newly Acquired |cffFF0000 {} |rhas received|cffFF0000 3 |rrandom enchantments!", name);
    else if (slotRand[1] != -1)
        chathandle.PSendSysMessage("Newly Acquired |cffFF0000 {} |rhas received|cffFF0000 2 |rrandom enchantments!", name);
    else if (slotRand[0] != -1)
        chathandle.PSendSysMessage("Newly Acquired |cffFF0000 {} |rhas received|cffFF0000 1 |rrandom enchantment!", name);
}
uint32 getRandEnchantment(Item* item)
{    
    uint32 itemClass = item->GetTemplate()->Class;
    uint32 itemQuality = item->GetTemplate()->Quality;
    std::string classQueryString = "";
    int rarityRoll = -1;
    uint8 tier = 0;
    switch (itemClass)
    {
        case 2:
            classQueryString = "WEAPON";
            break;
        case 4:
            classQueryString = "ARMOR";
            break;
    }
    if (classQueryString == "")
        return -1;
    switch (itemQuality)
    {
        case GREY:
            rarityRoll = rand_norm() * 25;
            break;
        case WHITE:
            rarityRoll = rand_norm() * 50;
            break;
        case GREEN:
            rarityRoll = 45 + (rand_norm() * 20);
            break;
        case BLUE:
            rarityRoll = 65 + (rand_norm() * 15);
            break;
        case PURPLE:
            rarityRoll = 80 + (rand_norm() * 14);
            break;
        case ORANGE:
            rarityRoll = 93;
            break;
    }
    if (rarityRoll < 0)
        return -1;
    if (rarityRoll <= 44)
        tier = 1;
    else if (rarityRoll <= 64)
        tier = 2;
    else if (rarityRoll <= 79)
        tier = 3;
    else if (rarityRoll <= 92)
        tier = 4;
    else
        tier = 5;
    QueryResult result = WorldDatabase.Query("SELECT `enchantID` FROM `item_enchantment_random_tiers` WHERE `tier`={} AND `exclusiveSubClass`=NULL AND exclusiveSubClass='{}' OR `class`='{}' OR `class`='ANY' ORDER BY RAND() LIMIT 1", tier, item->GetTemplate()->SubClass, classQueryString, classQueryString);
    if (!result)
        return 0;
    return result->Fetch()[0].Get<uint32>();
}
void RandomEnchantsPlayer::OnPlayerLogin(Player* player)
{
    if (sConfigMgr->GetOption<bool>("RandomEnchants.AnnounceOnLogin", true) && (sConfigMgr->GetOption<bool>("RandomEnchants.Enable", true)))
        ChatHandler(player->GetSession()).SendSysMessage(sConfigMgr->GetOption<std::string>("RandomEnchants.OnLoginMessage", "This server is running a RandomEnchants Module.").c_str());
}
void RandomEnchantsPlayer::OnPlayerLootItem(Player* player, Item* item, uint32 /*count*/, ObjectGuid /*lootguid*/)
{
    if (sConfigMgr->GetOption<bool>("RandomEnchants.OnLoot", true) && sConfigMgr->GetOption<bool>("RandomEnchants.Enable", true))
        rollPossibleEnchant(player, item);
}
void RandomEnchantsPlayer::OnPlayerCreateItem(Player* player, Item* item, uint32 /*count*/)
{
    if (sConfigMgr->GetOption<bool>("RandomEnchants.OnCreate", true) && (sConfigMgr->GetOption<bool>("RandomEnchants.Enable", true)))
        rollPossibleEnchant(player, item);
}
void RandomEnchantsPlayer::OnPlayerQuestRewardItem(Player* player, Item* item, uint32 /*count*/)
{
    if (sConfigMgr->GetOption<bool>("RandomEnchants.OnQuestReward", true) && (sConfigMgr->GetOption<bool>("RandomEnchants.Enable", true)))
        rollPossibleEnchant(player, item);
}
void RandomEnchantsPlayer::OnPlayerGroupRollRewardItem(Player* player, Item* item, uint32 /*count*/, RollVote /*voteType*/, Roll* /*roll*/)
{
    if (sConfigMgr->GetOption<bool>("RandomEnchants.OnGroupRoll", true) && (sConfigMgr->GetOption<bool>("RandomEnchants.Enable", true)))
        rollPossibleEnchant(player, item);
}