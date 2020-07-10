BEGIN TRANSACTION;

CREATE TABLE zs_state_frame (
    identifier_id INTEGER REFERENCES zs_identifier (id) ON DELETE CASCADE,
    sprite_base   TEXT,
    sprite_frames TEXT,
    duration      INTEGER
);

COMMIT TRANSACTION;
