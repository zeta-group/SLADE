BEGIN TRANSACTION;

CREATE TABLE zs_identifier (
    id        INTEGER PRIMARY KEY,
    source_id INTEGER REFERENCES zs_source (id) ON DELETE CASCADE,
    type_id   INTEGER,
    name      TEXT,
    parent_id INTEGER REFERENCES zs_identifier (id) ON DELETE CASCADE
);

CREATE INDEX zs_identifier_source_id ON zs_identifier (source_id);
CREATE INDEX zs_identifier_parent_id ON zs_identifier (parent_id);

COMMIT TRANSACTION;
