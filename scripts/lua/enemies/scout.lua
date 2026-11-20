function scout_has_target(context, parameters)
    if context.player_entity == nil then
        return false
    end

    local ecs = context.world.ecs
    if not ecs:is_entity_alive(context.player_entity)
        or not ecs:has_transform(context.entity)
        or not ecs:has_transform(context.player_entity) then
        return false
    end

    local position = ecs:get_transform(context.entity).position
    local target = ecs:get_transform(context.player_entity).position
    return (target - position):length() <= parameters.sight_range
end

function scout_chase(context, parameters)
    local ecs = context.world.ecs
    local position = ecs:get_transform(context.entity).position
    local target = ecs:get_transform(context.player_entity).position
    local direction = target - position
    if direction:length() > 0 then
        ecs:get_rigidbody(context.entity).velocity = direction:normalized() * parameters.speed
    else
        ecs:get_rigidbody(context.entity).velocity = sf.Vector2f(0, 0)
    end
    return "SUCCESS"
end

function scout_patrol(context, _)
    context.world.ecs:get_rigidbody(context.entity).velocity = sf.Vector2f(0, 18)
    return "SUCCESS"
end
