from copier_template_extensions import ContextHook


class Categories(ContextHook):
    def hook(self, context):
        context["categories"] = {
            "Application Development": "application_development",
            "Architecture-dependent Samples": "arch",
            "Basic": "basic",
            "Bluetooth": "bluetooth",
            "Boards": "boards",
            "C++": "cpp",
            "Data Structures": "data_structures",
            "Drivers": "drivers",
            "Hello World": "hello_world",
            "Kernel and Scheduler": "kernel",
            "External modules": "modules",
            "Networking": "net",
            "Philosophers": "philosophers",
            "POSIX API": "posix",
            "PSA": "psa",
            "Sensors": "sensor",
            "Shields": "shields",
            "Subsystems": "subsys",
            "Synchronization": "synchronization",
            "Sysbuild": "sysbuild",
            "Test": "test",
            "TF-M Integration": "tfm_integration",
            "Userspace": "userspace",
        }
