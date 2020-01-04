#
# This file is subject to the terms and conditions defined in
# file 'LICENSE', which is part of this source code package.
#

import unittest
import cnc


class CncTestCase(unittest.TestCase):
    def cnc_accepts_heartbeats(self):
        self.assertEqual(True, False)


if __name__ == '__main__':
    unittest.main()
