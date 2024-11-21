from twisterlib.mixins import DisablePyTestCollectionMixin


class TestClassToIgnore(DisablePyTestCollectionMixin):
    def test_to_ignore(self):
        assert False
