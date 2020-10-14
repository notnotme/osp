/*
 * This file is part of OSP (https://github.com/notnotme/osp).
 * Copyright (c) 2020 Romain Graillot
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <SDL2/SDL.h>
#include <ECS.h>
#include <glm/glm.hpp>

#include "../event/app/ScreenSizeChangeEvent.h"


class RenderSystem :
public ECS::EntitySystem,
public ECS::EventSubscriber<ScreenSizeChangeEvent>
{
public:
    RenderSystem(SDL_Window* window);
    virtual ~RenderSystem();

    virtual void configure(ECS::World* world) override;
    virtual void unconfigure(ECS::World* world) override;
    virtual void tick(ECS::World* world, float deltaTime) override;

    virtual void receive(ECS::World* world, const ScreenSizeChangeEvent& event) override;

private:
    SDL_Window* mWindow;
    glm::ivec2 mScreenSize;

    RenderSystem(const RenderSystem& copy);
};
