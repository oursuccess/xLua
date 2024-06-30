#pragma once

#include <vector>
#include <exception>
#include <stdexcept>

namespace CPL
{
	template<typename T>
	class Arena;

	template<typename T>
	class Idx {
	public:
		using ArenaClass = Arena<T>;

		Idx() : raw(0), arena(nullptr) {}
		Idx(uint32_t raw, ArenaClass* arena) : raw(raw), arena(arena) {}
		T& operator*() { return arena->Index(*this); }
		T* operator->() { return &arena->Index(*this); }
		ArenaClass* GetArena() { return arena; }

		uint32_t raw;

	private:
		ArenaClass* arena;
	};

	template<typename T>
	class Arena {
	public:
		T& Index(Idx<T> id) {
			if (id.raw < data.size()) return data[id.raw];
			throw std::runtime_error("index out of range in Arena::Index");
		}

		Idx<T> Alloc() {
			uint32_t rawid = static_cast<uint32_t>(data.size());
			data.emplace_back();
			return Idx<T>(rawid, this);
		}

		void Clear() {
			data.clear();
		}

	private:
		std::vector<T> data;
	};
}