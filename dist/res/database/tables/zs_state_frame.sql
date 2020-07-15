BEGIN TRANSACTION;

CREATE TABLE zs_state_frame (
    identifier_id INTEGER REFERENCES zs_identifier (id) ON DELETE CASCADE,
    sprite_base   TEXT,
    sprite_frames TEXT,
    duration      INTEGER
);

CREATE INDEX zs_state_frame_identifier_id ON zs_state_frame (identifier_id);

COMMIT TRANSACTION;
