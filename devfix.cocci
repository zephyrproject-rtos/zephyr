@r_base@
expression DE;
identifier ftag =~ "(?x)^
(name
|config_info
|driver_api
|driver_data
|device_pm_control
|pm
)$";
@@
DE->ftag

@r_param
 extends r_base@
identifier FN;
identifier D;
@@
FN(..., struct device *D, ...) {
 <...
 D->
+fixed->
 ftag
 ...>
}

@r_local
 extends r_base@
identifier D;
@@
 struct device *D;
 <...
 D->
+fixed->
 ftag
 ...>

@r_expr
 extends r_base@
expression E;
@@
 (struct device *)E->
+fixed->
 ftag
