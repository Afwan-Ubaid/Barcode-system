#pragma once
#include <SFML/Graphics.hpp>

namespace LoginTheme {
    // ── Original Bright Canvas & Artwork (Clean 16:9 Watercolor & Sketches) ──
    inline const sf::Color ParchmentCanvas  {243, 239, 230, 255}; // #F3EFE6
    inline const sf::Color ArtworkTint      {255, 255, 255, 255}; // 100% Full natural brightness & resolution

    // ── Header Typography (Above the card, on bright parchment) ──
    inline const sf::Color InkBrandTitle    {38,  36,  32,  255}; // #262420 Classic charcoal serif
    inline const sf::Color InkBrandSubtitle {95,  90,  82,  255}; // #5F5A52 Warm slate
    inline const sf::Color InkBrandCaption  {122, 116, 105, 255}; // #7A7469 Uppercase tracker

    // ── Dark Eye-Protected Frosted Glass Card (The Login Section Only) ──
    inline const sf::Color GlassCardFill    {26,  22,  19,  245}; // #1A1613 @ 96% alpha dark obsidian glass
    inline const sf::Color GlassCardShadow  {40,  35,  28,  75};  // Diffused warm sepia drop shadow
    inline const sf::Color GlassSpecularRim {195, 175, 145, 140}; // Warm bronze/gold rim

    // ── Card Internal Typography ──
    inline const sf::Color CardTitle        {250, 246, 238, 255}; // Soft ivory title inside card
    inline const sf::Color CardLabel        {195, 182, 166, 255}; // Warm cream input labels

    // ── Role Selector Pill ──
    inline const sf::Color RoleTrackBg      {36,  30,  26,  240}; // Recessed dark pill track
    inline const sf::Color RoleTrackBorder  {65,  56,  48,  200}; // Subtle track border
    inline const sf::Color RolePillAdmin    {165, 120, 78,  250}; // Warm antique bronze for Admin
    inline const sf::Color RolePillEmployee {98,  132, 90,  250}; // Athelia Sage for Employee
    inline const sf::Color RoleTextActive   {255, 255, 255, 255}; // Pure crisp white
    inline const sf::Color RoleTextIdle     {160, 148, 135, 255}; // Muted warm stone

    // ── Inputs & Interactive Controls ──
    inline const sf::Color InputFieldBg     {19,  16,  14,  235}; // Recessed dark input fill
    inline const sf::Color InputFieldFocus  {28,  23,  20,  250}; // Subtle brighten on focus
    inline const sf::Color InputBorderIdle  {72,  62,  54,  200}; // Soft stone border
    inline const sf::Color InputBorderFocus {126, 154, 118, 255}; // Sage-green focus ring
    inline const sf::Color InputText        {252, 250, 245, 255}; // Typed characters
    inline const sf::Color InputPlaceholder {118, 105, 92,  255}; // Hint text

    // ── Action Buttons ──
    inline const sf::Color LoginBtnFill     {105, 140, 98,  248}; // Athelia Sage frosted glass button
    inline const sf::Color LoginBtnHover    {125, 165, 116, 255}; // Luminous sage hover
    inline const sf::Color ResetBtnFill     {45,  38,  33,  235}; // Neutral warm stone reset
    inline const sf::Color ResetBtnHover    {62,  54,  46,  255}; // Reset hover

    // ── Alerts ──
    inline const sf::Color TerracottaAlert  {220, 88,  75,  255}; // Error notification
    inline const sf::Color SageVerified     {120, 190, 110, 255}; // Success notification
}
