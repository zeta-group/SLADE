BEGIN TRANSACTION;

CREATE TABLE zs_enumerator_value (
    identifier_id INTEGER REFERENCES zs_identifier (id) ON DELETE CASCADE,
    name          TEXT,
    value         INTEGER
);

CREATE INDEX zs_enumerator_value_identifier_id ON zs_enumerator_value (identifier_id);

COMMIT TRANSACTION;
