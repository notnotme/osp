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
#include "RenderSystem.h"

#include <glad/glad.h>

#include "../config.h"


RenderSystem::RenderSystem(SDL_Window* window) :
ECS::EntitySystem(),
mWindow(window)
{
}

RenderSystem::~RenderSystem()
{
    mWindow = nullptr;
}

void RenderSystem::configure(ECS::World* world)
{
    TRACE(">>>");

    // Subscribe for events
    world->subscribe<ScreenSizeChangeEvent>(this);
}

void RenderSystem::unconfigure(ECS::World* world)
{
    TRACE(">>>");

    // Unubscribe for events
    world->unsubscribe<ScreenSizeChangeEvent>(this);
}

void RenderSystem::tick(ECS::World* world, float deltaTime)
{
    glViewport(0, 0, mScreenSize.x, mScreenSize.y);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Do something that use the ECS components at least
}

void RenderSystem::receive(ECS::World* world, const ScreenSizeChangeEvent& event)
{
    TRACE("Received ScreenSizeChangeEvent: {:d}x{:d}.", event.size.x, event.size.y);
    mScreenSize = glm::ivec2(event.size.x, event.size.y);
}
