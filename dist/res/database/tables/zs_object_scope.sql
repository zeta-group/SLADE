BEGIN TRANSACTION;

CREATE TABLE zs_object_scope (
    id   INTEGER PRIMARY KEY,
    name TEXT
);

INSERT INTO zs_object_scope (id, name) VALUES (1, 'data');
INSERT INTO zs_object_scope (id, name) VALUES (2, 'play');
INSERT INTO zs_object_scope (id, name) VALUES (3, 'ui');

COMMIT TRANSACTION;
