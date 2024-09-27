import pytest
from pytest_embedded import Dut, log


from time import sleep

from pytest_embedded_serial import Serial

@pytest.mark.esp32
@pytest.mark.supported_targets
@pytest.mark.generic
@pytest.mark.unit
def test_hardware_test(dut: Dut) -> None:
    
    dut.expect_unity_test_output(timeout = 2000)

    dut.run_single_board_case("test restore serial monitor settings")
    dut.write('[ignore]')
    dut.expect('serial monitor settings was restored')

    dut.expect('esp:write text')
    dut.expect('esp:text')

    dut.expect('esp:write bin data')
    dut.expect('013')

    dut.expect('esp:write a byte')
    dut.expect('0')

    test_data = b'pytest:data'
    dut.expect('esp:expect data')
    dut.write(test_data)
    dut.expect('esp:ok')


    


