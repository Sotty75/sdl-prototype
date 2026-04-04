-- Player actor script
-- This script is loaded by the engine when a "player" template actor is spawned.
-- It must return a table with lifecycle callback functions.

local player = {}

function player.on_create(self)
    sot.log("Player created: " .. self.name .. " at (" .. self.x .. ", " .. self.y .. ")")
end

function player.on_update(self, dt)
    local speed = sot.actor.get_property(self.id, "speed") or 80
    local vx, vy = 0, 0

    if sot.input.is_pressed("move_left") then
        vx = -speed
    elseif sot.input.is_pressed("move_right") then
        vx = speed
    end

    if sot.input.is_just_pressed("jump") then
        local jump_force = sot.actor.get_property(self.id, "jump_force") or 200
        sot.physics.apply_impulse(self.id, 0, -jump_force)
    end

    sot.physics.set_velocity(self.id, vx, vy)
end

function player.on_destroy(self)
    sot.log("Player destroyed: " .. self.name)
end

return player
