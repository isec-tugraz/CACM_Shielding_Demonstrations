import stormpy
import stormpy.logic
from helpers.helper import get_example_path


class TestLabeling:
    def test_set_labeling(self):
        program = stormpy.parse_prism_program(get_example_path("dtmc", "die.pm"))
        model = stormpy.build_model(program)
        labeling = model.labeling
        assert "tmp" not in labeling.get_labels()
        assert not labeling.contains_label("tmp")
        labeling.add_label("tmp")
        assert labeling.contains_label("tmp")
        labeling.add_label_to_state("tmp", 0)
        assert labeling.has_state_label("tmp", 0)
        states = labeling.get_states("tmp")
        assert states.get(0)
        states.set(3)
        labeling.set_states("tmp", states)
        assert labeling.has_state_label("tmp", 3)

    def test_label(self):
        program = stormpy.parse_prism_program(get_example_path("dtmc", "die.pm"))
        formulas = stormpy.parse_properties_for_prism_program("P=? [ F \"one\" ]", program)
        model = stormpy.build_model(program, formulas)
        labeling = model.labeling
        labels = labeling.get_labels()
        assert len(labels) == 3
        assert "init" in labels
        assert "one" in labels
        assert "init" in model.labels_state(0)
        assert "init" in labeling.get_labels_of_state(0)
        assert "one" in model.labels_state(7)
        assert "one" in labeling.get_labels_of_state(7)

    def test_label_parametric(self):
        program = stormpy.parse_prism_program(get_example_path("pdtmc", "brp16_2.pm"))
        formulas = stormpy.parse_properties_for_prism_program("P=? [ F s=5 ]", program)
        model = stormpy.build_parametric_model(program, formulas)
        labels = model.labeling.get_labels()
        assert len(labels) == 3
        assert "init" in labels
        assert "(s = 5)" in labels
        assert "init" in model.labels_state(0)
        assert "(s = 5)" in model.labels_state(28)
        assert "(s = 5)" in model.labels_state(611)
        initial_states = model.initial_states
        assert len(initial_states) == 1
        assert 0 in initial_states
