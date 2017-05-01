

DEMO

    
    


CODE

    new (1st party):
        ga_audio_component
            - component for playing a wav file on loop at an entities 3D position
        ga_kb_move_component
            - component for keyboard control of an entities position
        ga_listener_component
            - component for determining how sound from registered audio components 
              should play; based on the listener and audio components' entitiy positions relative to static colliders in ga_physics_world 


    modified:
        main
            - scene creation
            - SoLoud initialization 
        ga_debug_geometry
            - debug line and 'star'
        ga_intersection
            - plane and oobb raycasting
        ga_intersection_tests
            - basic unit tests for plane and oobb raycast intersection functions
        ga_physics_world
            - racyast_all
            - get_mesh_corners gets corner positions of oobb shapes in _bodies
        ga_vec3f
            - hash function and == operator for use in comparing mesh corners

