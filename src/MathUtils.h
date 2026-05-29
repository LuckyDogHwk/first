#pragma once

#include <SFML/Graphics.hpp>
#include <cmath>
#include <algorithm>
#include <string>

inline float MassToRadius(float mass)
{
    return std::sqrt(mass / 3.14159265f);
}

inline float RadiusToMass(float radius)
{
    return 3.14159265f * radius * radius;
}

inline sf::Vector2f Normalize(const sf::Vector2f& v)
{
    const float len = std::sqrt(v.x * v.x + v.y * v.y);
    if (len < 0.0001f)
        return { 0.f, 0.f };
    return { v.x / len, v.y / len };
}

inline float Length(const sf::Vector2f& v)
{
    return std::sqrt(v.x * v.x + v.y * v.y);
}

inline sf::Vector2f ClampToWorld(const sf::Vector2f& pos, float radius, float worldW, float worldH)
{
    return {
        std::clamp(pos.x, radius, worldW - radius),
        std::clamp(pos.y, radius, worldH - radius)
    };
}

inline sf::Color RandomBrightColor(unsigned int seed)
{
  seed = seed * 1103515245u + 12345u;
  const float hue = static_cast<float>(seed % 360);
  const float s = 0.65f;
  const float v = 0.92f;

  const float c = v * s;
  const float x = c * (1.f - std::fabs(std::fmod(hue / 60.f, 2.f) - 1.f));
  const float m = v - c;

  float r = 0.f, g = 0.f, b = 0.f;
  if (hue < 60.f) { r = c; g = x; }
  else if (hue < 120.f) { r = x; g = c; }
  else if (hue < 180.f) { g = c; b = x; }
  else if (hue < 240.f) { g = x; b = c; }
  else if (hue < 300.f) { r = x; b = c; }
  else { r = c; b = x; }

  return sf::Color(
      static_cast<std::uint8_t>((r + m) * 255.f),
      static_cast<std::uint8_t>((g + m) * 255.f),
      static_cast<std::uint8_t>((b + m) * 255.f));
}

inline sf::String ToSfString(const std::string& utf8)
{
    // 使用指针而不是迭代器，避免 Debug 模式下的断言失败
    return sf::String::fromUtf8(utf8.data(), utf8.data() + utf8.size());
}

// sf::String 转 UTF-8 std::string
inline std::string ToUtf8String(const sf::String& str)
{
    std::string result;
    for (size_t i = 0; i < str.getSize(); ++i)
    {
        sf::Uint32 c = str[i];
        if (c < 0x80)
        {
            result += static_cast<char>(c);
        }
        else if (c < 0x800)
        {
            result += static_cast<char>(0xC0 | (c >> 6));
            result += static_cast<char>(0x80 | (c & 0x3F));
        }
        else if (c < 0x10000)
        {
            result += static_cast<char>(0xE0 | (c >> 12));
            result += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (c & 0x3F));
        }
        else
        {
            result += static_cast<char>(0xF0 | (c >> 18));
            result += static_cast<char>(0x80 | ((c >> 12) & 0x3F));
            result += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (c & 0x3F));
        }
    }
    return result;
}

// 计算点到线段的距离
inline float PointToSegmentDistance(const sf::Vector2f& p, const sf::Vector2f& a, const sf::Vector2f& b)
{
    sf::Vector2f ab = b - a;
    sf::Vector2f ap = p - a;
    float t = std::clamp((ap.x * ab.x + ap.y * ab.y) / (ab.x * ab.x + ab.y * ab.y), 0.f, 1.f);
    sf::Vector2f closest = a + ab * t;
    return Length(p - closest);
}
