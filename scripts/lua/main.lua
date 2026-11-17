game.event_handler:on_key_pressed(function(event)
    if event.code == sf.Keyboard.Key.Escape then
        game:request_close()
    end
end)
