BEGIN TRANSACTION;

CREATE TABLE zs_identifier (
    id        INTEGER PRIMARY KEY,
    source_id INTEGER REFERENCES zs_source (id) ON DELETE CASCADE,
    type_id   INTEGER,
    name      TEXT,
    parent_id INTEGER REFERENCES zs_identifier (id) ON DELETE CASCADE
);

COMMIT TRANSACTION;
