function knockback_modifier_on_collision(context)
    local ecs = context.world.ecs
    if ecs:is_entity_alive(context.target_entity)
        and ecs:has_rigidbody(context.target_entity)
        and ecs:has_transform(context.target_entity) then
        local projectile_position = ecs:get_transform(context.projectile_entity).position
        local target_position = ecs:get_transform(context.target_entity).position
        local direction = target_position - projectile_position
        local body = ecs:get_rigidbody(context.target_entity)
        if direction:length() > 0 then
            body.velocity = body.velocity + direction:normalized() * 120
        end
    end
end
