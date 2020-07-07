BEGIN TRANSACTION;

CREATE TABLE zs_enumerator_value (
    identifier_id INTEGER REFERENCES zs_identifier (id) ON DELETE CASCADE,
    name          TEXT,
    value         INTEGER
);

COMMIT TRANSACTION;
