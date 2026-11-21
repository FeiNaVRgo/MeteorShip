function pulse_turret_on_fire(world, owner, origin, aim_angle)
    if world.ecs:is_entity_alive(owner) then
        -- print("Pulse turret fired from entity " .. owner)
    end
    return true
end
