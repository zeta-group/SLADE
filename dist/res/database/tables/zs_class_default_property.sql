BEGIN TRANSACTION;

CREATE TABLE zs_class_default_property (
    identifier_id INTEGER REFERENCES zs_identifier (id) ON DELETE CASCADE,
    name          TEXT,
    value         TEXT
);

COMMIT TRANSACTION;
